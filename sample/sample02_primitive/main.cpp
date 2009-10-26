/*
    Copyright (c) 2007-2009 Takashi Kitao
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    `  notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
    `  notice, this list of conditions and the following disclaimer in the
    `  documentation and/or other materials provided with the distribution.

    3. The name of the author may not be used to endorse or promote products
    `  derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "pogolyn_main.h"


void newPogolynLogo(const char** cmd, u8 cmd_num);
void newTriangle();
void newAmeba(r32 x, r32 y, pgCol color);


static const char* s_cmd[] =
{
    "P: CHANGE PRIM MODE", //
    "F: TOGGLE FULLSCREEN", //
    "Q: QUIT"
};


pgMain()
{
    pgCreatePogolyn("Sample02 - Primitive", 640, 480, 60);

    pgResMgr::loadResource("../data/pogolyn_logo_71x14.png", true);
    pgResMgr::loadResource("../data/coin_400x200.png", true);

    newPogolynLogo(s_cmd, sizeof(s_cmd) / sizeof(char*));
    newTriangle();
    newAmeba(0.0f, 0.0f, pgCol(128, 128, 128));
    newAmeba(-200.0f, 120.0f, pgCol(255, 128, 64));
    newAmeba(0.0f, 240.0f, pgCol(64, 255, 128));
    newAmeba(200.0f, 120.0f, pgCol(128, 64, 255));
    newAmeba(-200.0f, -120.0f, pgCol(255, 64, 128));
    newAmeba(0.0f, -240.0f, pgCol(128, 255, 64));
    newAmeba(200.0f, -120.0f, pgCol(64, 128, 255));

    pgStartPogolyn();
    pgDestroyPogolyn();
}