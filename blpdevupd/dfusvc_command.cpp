// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfusvc_command.cpp
#include "dfusvc_command.h"

#include <iostream>

#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>



namespace dfusvc
{

OpenRequest::OpenRequest()
: d_pid(0)
, d_vid(0)
{
}

OpenRequest::OpenRequest(uint16_t vid, uint16_t pid)
: d_pid(pid)
, d_vid(vid)
{

}

int OpenRequest::serialize(std::vector<uint8_t>& raw)
{
    try {
        std::ostringstream oss;
        boost::property_tree::ptree pt;
        pt.put("type", e_open);
        pt.put("vid", d_vid);
        pt.put("pid", d_pid);

        std::ostringstream buf;
        boost::property_tree::write_json(buf, pt, false);

        raw.clear();
        for (auto it : buf.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

uint16_t OpenRequest::pid(void)
{
    return uint16_t(d_pid);
}

uint16_t OpenRequest::vid(void)
{
    return uint16_t(d_vid);
}

int OpenRequest::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_pid = pt_req.get<uint16_t>("pid");
        d_vid = pt_req.get<uint16_t>("vid");

        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

OpenResponse::OpenResponse(void)
{
}

OpenResponse::OpenResponse(int handle)
: d_handle(handle)
{
}

int OpenResponse::serialize(std::vector<uint8_t>& raw)
{
    try {
        std::ostringstream oss;
        boost::property_tree::ptree pt;
        pt.put("type", e_open);
        pt.put("handle", d_handle);

        std::ostringstream buf;
        boost::property_tree::write_json(buf, pt, false);

        raw.clear();
        for (auto it : buf.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

int OpenResponse::handle(void)
{
    return d_handle;
}

int OpenResponse::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_handle = pt_req.get<int>("handle");

        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}


DownloadRequest::DownloadRequest()
: d_handle(-1)
{
    d_data.clear();
}
DownloadRequest::DownloadRequest(int handle, std::vector<uint8_t>& data)
: d_handle(handle)
, d_data(data)
{
}
int DownloadRequest::serialize(std::vector<uint8_t>& raw)
{
    try {
        std::ostringstream oss;
        boost::algorithm::hex(d_data.begin(), d_data.end(), std::ostream_iterator<char>(oss));
        boost::property_tree::ptree pt;
        pt.put("type", e_download);
        pt.put("handle", d_handle);
        pt.put("data", oss.str());

        std::ostringstream buf;
        boost::property_tree::write_json(buf, pt, false);

        raw.clear();
        for (auto it : buf.str()) {
            raw.push_back(it);
        }
        return 0;
    } catch (const std::exception& exc) {
        return -1;
    }
}
int DownloadRequest::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_handle = pt_req.get<int>("handle");
        std::string& heximage = pt_req.get<std::string>("data");

        boost::algorithm::unhex(heximage.c_str(), std::back_inserter(d_data));
        return 0;
    } catch (const std::exception& exc) {
        return -1;
    }
}

int DownloadRequest::handle(void)
{
    return d_handle;
}

std::vector<uint8_t> DownloadRequest::data(void)
{
    return d_data;
}

DownloadResponse::DownloadResponse(size_t sent, 
                                   size_t total,
                                   int handle,
                                   bool last)
: d_sent(sent)
, d_total(total)
, d_last(last)
, d_handle(handle)
{
}

int DownloadResponse::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_download);
        pt_resp.put("lastResponse", d_last);
        pt_resp.put("bytes_downloaded", d_sent);
        pt_resp.put("bytes_total", d_total);
        pt_resp.put("handle", d_handle);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    } catch (const std::exception& exc) {
        return -1;
    }
}
int DownloadResponse::handle(void)
{
    return d_handle;
}
int DownloadResponse::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_sent = pt_req.get<size_t>("bytes_downloaded");
        d_total = pt_req.get<size_t>("bytes_total");
        d_last = pt_req.get<bool>("lastResponse");
        d_handle = pt_req.get<int>("handle");
        return 0;
    } catch (const std::exception& exc) {
        return -1;
    }
}
size_t DownloadResponse::sent(void)
{
    return d_sent;
}
size_t DownloadResponse::total(void)
{
    return d_total;
}
bool DownloadResponse::final(void)
{
    return d_last;
}
int CommandRequestUtil::getCommandType(CommandType& type, std::vector<uint8_t> raw)
{
    boost::property_tree::ptree pt_req;
    std::istringstream is(std::string(raw.begin(), raw.end()));
    read_json(is, pt_req);

    type = static_cast<CommandType>(pt_req.get<int>("type"));

    return 0;
}

CloseRequest::CloseRequest(void)
:d_handle(0)
{
}

CloseRequest::CloseRequest(int handle)
:d_handle(handle)
{
}



int CloseRequest::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_close);
        pt_resp.put("handle", d_handle);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

int CloseRequest::handle(void)
{
    return d_handle;
}

int CloseRequest::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_handle = pt_req.get<int>("handle");
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

CloseResponse::CloseResponse(void)
{

}

int CloseResponse::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_close);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

int CloseResponse::deserialize(std::vector<uint8_t> raw)
{
    return 0;
}

ErrorResponse::ErrorResponse(ErrorType err, 
                             std::string errString)
:d_err(err)
,d_string(errString)
{
    
}

int ErrorResponse::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_error);
        pt_resp.put("errorCode", d_err);
        pt_resp.put("errorString", d_string);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

ErrorType ErrorResponse::errorCode(void)
{
    return ErrorType();
}

std::string ErrorResponse::errorString(void)
{
    return std::string();
}

int ErrorResponse::deserialize(std::vector<uint8_t> raw)
{
    try {
        boost::property_tree::ptree pt_req;
        std::istringstream is(std::string(raw.begin(), raw.end()));
        read_json(is, pt_req);

        d_err = static_cast<ErrorType>(pt_req.get<int>("errorCode"));
        d_string = pt_req.get<std::string>("errorString");
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }

}

TerminateRequest::TerminateRequest(void)
{
}

int TerminateRequest::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_terminate);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

int TerminateRequest::deserialize(std::vector<uint8_t> raw)
{
    return 0;
}

TerminateResponse::TerminateResponse(void)
{
}

int TerminateResponse::serialize(std::vector<uint8_t>& raw)
{
    try {
        boost::property_tree::ptree pt_resp;
        pt_resp.put("type", e_terminate);
        std::ostringstream sresponse;
        boost::property_tree::write_json(sresponse, pt_resp, false);
        raw.clear();
        for (auto it : sresponse.str()) {
            raw.push_back(it);
        }
        return 0;
    }
    catch (const std::exception& exc) {
        return -1;
    }
}

int TerminateResponse::deserialize(std::vector<uint8_t> raw)
{
    return 0;
}

}
