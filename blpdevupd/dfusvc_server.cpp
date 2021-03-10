// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfusvc_server.cpp
#include "dfusvc_server.h"

#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <streambuf>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/unordered_map.hpp>
#include "dfutransport.h"

namespace dfusvc {

static boost::unordered_map<int, boost::shared_ptr<DFUTransport>> s_deviceMap;
static int s_gcount = 1;

DFUServiceServer::DFUServiceServer(std::string name)
{

    if (0 == name.size()) {
        d_pipeName += "dfusvcpipe";
    }
    else {
        d_pipeName += name;
    }

    std::cout << "Pipe Server: Main thread awaiting client connection on" << d_pipeName << std::endl;
    d_hPipe = CreateNamedPipe(
        d_pipeName.c_str(),             // pipe name 
        PIPE_ACCESS_DUPLEX,       // read/write access 
        PIPE_TYPE_MESSAGE |       // message type pipe 
        PIPE_READMODE_MESSAGE |   // message-read mode 
        PIPE_WAIT,                // blocking mode 
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        k_bufferSize,                  // output buffer size 
        k_bufferSize,                  // input buffer size 
        0,                        // client time-out 
        NULL);                    // default security attribute 
}

DFUServiceServer::~DFUServiceServer()
{
    DisconnectNamedPipe(d_hPipe);
    CloseHandle(d_hPipe);
}

int DFUServiceServer::waitForConnection(void)
{
    BOOL   fConnected = FALSE;
    if (d_hPipe == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateNamedPipe failed, GLE= " << GetLastError() << std::endl;
        return -1;
    }
    // Wait for the client to connect; if it succeeds, 
    // the function returns a nonzero value. If the function
    // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
    fConnected = ConnectNamedPipe(d_hPipe, NULL) ?
        TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (fConnected) {
        std::cout << "Client connected, creating a processing thread." << std::endl;
        return 0;
    }
    else {
        // The client could not connect, so close the pipe. 
        //CloseHandle(d_hPipe);
        return -1;
    }

}

int DFUServiceServer::processRequest(void)
{
    // Print verbose messages. In production code, this should be for debugging only.
    std::cout << "Start process request." << std::endl;

    // Read client request from pipes
    std::vector<uint8_t> request;
    auto ret = getRawRequest(request);
    if (ret != 0) {
        return -1;
    }

    // Create server command according to request
    auto cmd = dfusvc::ServerCommandFactory::makeServerCommand(request);

    if (nullptr == cmd) {
        return -1;
    }

    // Execute command
    ret = cmd->execute(boost::bind(&DFUServiceServer::sendResponse, this, _1));

    // Flush the pipe to allow the client to read the pipe's contents 
    // before disconnecting. Then disconnect the pipe, and close the 
    // handle to this pipe instance. 

    FlushFileBuffers(d_hPipe);

    std::cout << "Finish process request." << std::endl;

    return ret;
}

int DFUServiceServer::getRawRequest(std::vector<uint8_t>& request)
{

    // This is from MS document:
    // To create the pipe handle in message - read mode, specify 
    // PIPE_READMODE_MESSAGE.Data is read from the pipe as a stream of 
    // messages.A read operation is completed successfully only when the 
    // entire message is read.If the specified number of bytes to read is 
    // less than the size of the next message, the function reads as much 
    // of the message as possible before returning zero(the GetLastError 
    // function returns ERROR_MORE_DATA).The remainder of the message can 
    // be read using another read operation. at: 
    // https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-type-read-and-wait-modes
    while (1) {
        std::vector<char> buf(k_bufferSize, '\0');
        DWORD cbBytesRead = 0;
        // Read client requests from the pipe. This simplistic code only allows messages
        // up to BUFSIZE characters in length.
        auto fSuccess = ReadFile(

            d_hPipe,        // handle to pipe 
            buf.data(),    // buffer to receive data 
            buf.size(), // size of buffer 
            &cbBytesRead, // number of bytes read 
            NULL);        // not overlapped I/O 

        std::cout << "bytes read: " << cbBytesRead << std::endl;
        if (fSuccess) {
            std::cout << "Append packet to request buffer: " << std::endl;
            request.insert(request.end(), buf.begin(), buf.begin() + cbBytesRead);
            break;
        }
        auto error = GetLastError();
        if (ERROR_MORE_DATA == error) {
            std::cout << "Append packet to request buffer: " << std::endl;
            request.insert(request.end(), buf.begin(), buf.begin() + cbBytesRead);
            continue;
        }
        else {
            std::cout << "InstanceThread ReadFile failed, GLE= " << error << std::endl;
            return -1;
        }
    }
    return 0;
}

int DFUServiceServer::sendResponse(dfusvc::CommandResponse& resp)
{
    DWORD cbWritten = 0;
    BOOL fSuccess = FALSE;
    std::vector<uint8_t> response;

    resp.serialize(response);

    // Write the reply to the pipe. 
    fSuccess = WriteFile(
        d_hPipe,        // handle to pipe 
        response.data(),     // buffer to write from 
        response.size(), // number of bytes to write 
        &cbWritten,   // number of bytes written 
        NULL);        // not overlapped I/O 

    if (!fSuccess || response.size() != cbWritten)
    {
        std::cout << "InstanceThread WriteFile failed, GLE= " << GetLastError() << std::endl;
        return -1;
    }
    return 0;
}



ServerDownloadCommand::ServerDownloadCommand(std::vector<uint8_t> raw)
    :d_rawRequest(raw)
{
}

int ServerDownloadCommand::execute(std::function<int(dfusvc::CommandResponse&)> fRespSend)
{

    DownloadRequest req;
    req.deserialize(d_rawRequest);

    if (nullptr == fRespSend) {
        return -1;
    }

    boost::shared_ptr<DFUTransport> dfu;

    if (s_deviceMap.find(req.handle()) != s_deviceMap.end()) {
        dfu = s_deviceMap.at(req.handle());
    }
    else {
        return fRespSend(dfusvc::ErrorResponse(e_transportErr, "Invalid handle"));
    }

    int bytes_sent = 0;
    int bytes_total = 0;
    
    auto download_cb = [&](int sent, int total) {
        // Send progress update message as none-final to client
        bytes_sent = sent;
        bytes_total = total;
        fRespSend(dfusvc::DownloadResponse(sent, total, req.handle()));
    };

    int ret = 0;

    // Download firmware into device
    if ((ret = dfu->download(req.data(), download_cb)) < 0) {
        std::cout << "Fail to download firmware" << std::endl;
    }

    // Check whether the full image is downloaded
    if (ret == req.data().size()) {
        return fRespSend(dfusvc::DownloadResponse(bytes_sent, bytes_total, req.handle(), true));
    } else {
        return fRespSend(dfusvc::DownloadResponse(bytes_sent, bytes_total, req.handle(), true));
    }
}

std::shared_ptr<ServerCommand> ServerCommandFactory::makeServerCommand(std::vector<uint8_t> raw)
{
    CommandType type;
    CommandRequestUtil::getCommandType(type, raw);
    switch (type)
    {
    case e_open:
        return std::make_shared<ServerOpenCommand>(raw);
    case e_download:
        return std::make_shared<ServerDownloadCommand>(raw);
    case e_close:
        return std::make_shared<ServerCloseCommand>(raw);
    case e_terminate:
        return std::make_shared<ServerTerminateCommand>(raw);
    default:
        return nullptr;
    }
}

ServerOpenCommand::ServerOpenCommand(std::vector<uint8_t> raw)
: d_rawRequest(raw)
{

}

int ServerOpenCommand::execute(std::function<int(dfusvc::CommandResponse&)> fRespSend)
{
    OpenRequest req;
    req.deserialize(d_rawRequest);

    boost::shared_ptr<DFUTransport> dfu = boost::make_shared<DFUTransport>();

    if (dfu->init() < 0) {
        return fRespSend(dfusvc::ErrorResponse(e_transportErr, "Fail to init dfu transport"));
    }

    std::cout << "Looking for DFU device...." << std::endl;

    // Search DFU device for 5 seconds
    int delay = 5;
    while (delay--) {
        std::cout << "Searching for DFU device...." << std::endl;
        Sleep(1000);
        if (dfu->open(req.vid(), req.pid()) == 0) {
            std::cout << "Find DFU device" << std::endl;
            break;
        }
    }

    if (delay <= 0) {
        return fRespSend(dfusvc::ErrorResponse(e_transportErr, "No DFU device found"));
    }
    
    int handle = s_gcount++;
    s_deviceMap[handle] = dfu;

    return fRespSend(dfusvc::OpenResponse(handle));
}

ServerCloseCommand::ServerCloseCommand(std::vector<uint8_t> raw)
: d_rawRequest(raw)
{
}

int ServerCloseCommand::execute(std::function<int(dfusvc::CommandResponse&)> fRespSend)
{
    CloseRequest req;
    req.deserialize(d_rawRequest);

    if (nullptr == fRespSend) {
        return -1;
    }

    boost::shared_ptr<DFUTransport> dfu;

    if (s_deviceMap.find(req.handle()) != s_deviceMap.end()) {
        dfu = s_deviceMap.at(req.handle());
    }
    else {
        return fRespSend(dfusvc::ErrorResponse(e_transportErr, "Invalid handle"));
    }

    if (0 != dfu->close()) {
        return fRespSend(dfusvc::ErrorResponse(e_transportErr, "Can not close device"));
    }
    else {
        return fRespSend(dfusvc::CloseResponse());
    }
}

ServerTerminateCommand::ServerTerminateCommand(std::vector<uint8_t> raw)
: d_rawRequest(raw)
{
}

int ServerTerminateCommand::execute(std::function<int(dfusvc::CommandResponse&)> fRespSend)
{
    TerminateRequest req;
    int ret = -1;
    if (0 == req.deserialize(d_rawRequest)) {
        ret = -2;
    }

    if (nullptr == fRespSend) {
        return -1;
    }
    
    if (0 != fRespSend(dfusvc::TerminateResponse())) {
        return -1;
    }

    return ret;
}

}
