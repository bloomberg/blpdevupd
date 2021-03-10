// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfusvc_command.h
#ifndef DFUSVC_COMMAND_H
#define DFUSVC_COMMAND_H

#include <vector>
#include <memory>
#include <string>
#include <dfutransport.h>

namespace dfusvc {

enum CommandType{
    e_unknown = 0,
    e_open,
    e_download,
    e_close,
    e_error,
    e_terminate
};

enum ErrorType {
    e_noError = 0,
    e_transportErr,
    e_timeoutErr,
    e_deviceErr,
    e_commErr
};



                        // =====================
                        // class CommandRequest
                        // =====================

class CommandRequest 
{
// Interface class for all client to server requests
public:
    virtual int serialize(std::vector<uint8_t>& raw) = 0;
        // serialize message function
    virtual int deserialize(std::vector<uint8_t> raw) = 0;
        // Deserialize message function
};

class CommandRequestUtil
{
// Command Request util class
public:
    static int getCommandType(CommandType& type, std::vector<uint8_t> raw);
        // Extract command type from Command Request
};

class CommandResponse
{
// Base class for all server to client response
public:
    virtual int serialize(std::vector<uint8_t>& raw) = 0;
        // serialize message function
    virtual int deserialize(std::vector<uint8_t> raw) = 0;
        // Deserialize message function
};

class OpenRequest : public CommandRequest
{
// DFU open device request
private:
    // DATA
    uint16_t d_pid;
    uint16_t d_vid;
public:
    // CREATORS
    OpenRequest();
    OpenRequest(uint16_t vid, uint16_t pid);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function
    uint16_t pid(void);
        // pid field accessor
    uint16_t vid(void);
        // vid field accessors
    
    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};


class OpenResponse : public CommandResponse
{
    // DFU download response
private:
    // DATA
    int d_handle;
public:
    // CREATORS
    OpenResponse(void);
    OpenResponse(int handle);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function

    int handle(void);
        // Device handle accessor

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};

class DownloadRequest : public CommandRequest
{
// DFU download request
private:
    // DATA
    int d_handle;
    std::vector<uint8_t> d_data;
public:
    // CREATORS
    DownloadRequest();
    DownloadRequest(int handle, std::vector<uint8_t>& data);
    
    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function
    int handle(void);
        // handle field accessor
    std::vector<uint8_t> data(void);
        // data field accessor

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function

};

class DownloadResponse : public CommandResponse
{
// DFU download response
private:
    // DATA
    int d_handle;
    size_t d_sent;
    size_t d_total;
    bool d_last;
public:
    // CREATORS
    DownloadResponse(size_t sent = 0,
                     size_t total = 0,
                     int handle = 0,
                    bool last = false);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function
    int handle(void);
        // Return the handle of device
    size_t sent(void);
        // Return bytes downloaded into the device
    size_t total(void);
        // Return total length of data intended to download into device
    bool final(void);
        // Return true if this is the last response from server
        // false if this is not the last response

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};

class CloseRequest : public CommandRequest
{
    // DFU open device request
private:
    // DATA
    int d_handle;
public:
    // CREATORS
    CloseRequest(void);
    CloseRequest(int handle);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function
    int handle(void);
        // Handle accessor

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};


class CloseResponse : public CommandResponse
{
// DFU close device response

public:
    // CREATORS
    CloseResponse(void);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
        // serialize message function

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};

class TerminateRequest : public CommandRequest
{
// Request to terminate the servers
private:
    // DATA
    int d_handle;
public:
    // CREATORS
    TerminateRequest(void);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
    // serialize message function

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
    // Deserialize message function
};


class TerminateResponse : public CommandResponse
{
    // DFU close device response

public:
    // CREATORS
    TerminateResponse(void);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
    // serialize message function

//MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
    // Deserialize message function
};

class ErrorResponse : public CommandResponse
{
// DFU close device response
private:
    ErrorType d_err;
    std::string d_string;
public:
    // CREATORS
    ErrorResponse(ErrorType err, std::string errString);

    // ACCESSORS
    int serialize(std::vector<uint8_t>& raw) override;
    // serialize message function

    ErrorType errorCode(void);
        // Get error code

    std::string errorString(void);
        // Get error string

    //MANIPULTORS
    int deserialize(std::vector<uint8_t> raw) override;
        // Deserialize message function
};

}

#endif //DFUSVC_COMMAND_H
