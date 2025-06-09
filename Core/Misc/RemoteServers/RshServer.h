// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "SocketServer.h"

namespace vamiga {

class RshServer final : public SocketServer {

public:
    
    using SocketServer::SocketServer;

    RshServer& operator= (const RshServer& other) {

        SocketServer::operator = (other);
        return *this;
    }


    //
    // Methods from CoreObject
    //
    
private:

    void _dump(Category category, std::ostream &os) const override;


    //
    // Methods from RemoteServer
    //

public:

    // bool shouldRun() override { return true; }


    //
    // Methods from SocketServer
    //

    
    string doReceive() throws override;
    void doProcess(const string &packet) throws override;
    void doSend(const string &packet)throws  override;
    void didStart() override;
    void didConnect() override;
};

}
