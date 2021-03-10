// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfusvc_server.h
#ifndef DFUSVC_SERVER_H
#define DFUSVC_SERVER_H

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "dfusvc_command.h"

namespace dfusvc
{

                            // =======================
                            // class DFUServiceServer
                            // =======================

class DFUServiceServer
{
private:
    // TYPES
    enum {
        k_bufferSize = 4096
    };
    std::string d_pipeName = "\\\\.\\pipe\\";
    // DATA
    HANDLE d_hPipe;
    // MANIPULTORS
    int getRawRequest(std::vector<uint8_t>& request);
    // This function read a full message from pipe
    int sendResponse(dfusvc::CommandResponse& resp);
    // This function send the whole response message to pipe
public:
    // CREATORS
    DFUServiceServer(std::string name);
    ~DFUServiceServer();
    // MANIPULTORS
    int waitForConnection(void);
        // THis function blocks until a client is connect to the server
    int processRequest(void);
        // This function process a client request synchronously 

};


                        // ====================
                        // class ServerCommand
                        // ====================

class ServerCommand
{
// This is the abstract base class for all server side commands class
// We use command design pattern here
public:
    virtual int execute(std::function<int(dfusvc::CommandResponse&)> fRespSend) = 0;
        // Abstract command execution function
};


                    // ========================
                    // class ServerOpenCommand
                    // ========================

class ServerOpenCommand : public ServerCommand
{
// This function handls all server side open device command related operation
// including decoding and execution.
private:
    std::vector<uint8_t> d_rawRequest;
    // The raw command request sent by client.
public:
    // CREATORS
    ServerOpenCommand(std::vector<uint8_t> raw);
    // Default constructor

    virtual int execute(std::function<int(dfusvc::CommandResponse&)> fRespSend) override;
    // This function opens a DFU device if it enumerates under local USB interface.
    // It returns a handle to the opened device
};

                    // ============================
                    // class ServerDownloadCommand
                    // ============================

class ServerDownloadCommand : public ServerCommand
{
// This function handls all server side download command related operation
// including decoding and execution.
private:
    std::vector<uint8_t> d_rawRequest;
        // The raw command request sent by client.
public:
    // CREATORS
    ServerDownloadCommand(std::vector<uint8_t> raw);
        // Default constructor

    virtual int execute(std::function<int(dfusvc::CommandResponse&)> fRespSend) override;
        // This function download binary into it according to the device according to the
        // handle in the client raw request. It will send mutilpe 
        // downloadResponses. The last response will have "last" field marked as true; other
        // responses will have "last" field as false.
};


class ServerCloseCommand : public ServerCommand
{
    // This function handls all server side close device command related operation
    // including decoding and execution.
private:
    std::vector<uint8_t> d_rawRequest;
    // The raw command request sent by client.
public:
    // CREATORS
    ServerCloseCommand(std::vector<uint8_t> raw);
    // Default constructor

    virtual int execute(std::function<int(dfusvc::CommandResponse&)> fRespSend) override;
    // This command close a DFU devicce according to the handle.
};


class ServerTerminateCommand : public ServerCommand
{
    // This function handls all server serviceTerminate request
private:
    std::vector<uint8_t> d_rawRequest;
    // The raw command request sent by client.
public:
    // CREATORS
    ServerTerminateCommand(std::vector<uint8_t> raw);
    // Default constructor

    virtual int execute(std::function<int(dfusvc::CommandResponse&)> fRespSend) override;
    // This command close a DFU devicce according to the handle.
};

                        // ===========================
                        // class ServerCommandFactory
                        // ===========================

class ServerCommandFactory
{
// ServerCommand Factory class. This class create concrete server command objects according
// to the type field in message.
public:
    static std::shared_ptr<ServerCommand> makeServerCommand(std::vector<uint8_t> raw);
        //Factory method create concrete objects
};
}


#endif //DFUSVC_SERVER_H
