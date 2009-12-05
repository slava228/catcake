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


#ifdef CK_ANDROID


#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
//#include <X11/Xlib.h>
//#include <X11/extensions/xf86vmode.h>
//#include <GL/glx.h>

#include "ck_low_level_api.h"

#include "ck_sys_all.h" // for ckSysMgr::SysFlag and ckSysMgr::sprintf
#include "ck_key_all.h" // for ckKeyMgr::KeyType


static ckLowLevelAPI::KeyEventHandler s_key_event_handler = NULL;
static ckLowLevelAPI::MouseEventHandler s_mouse_event_handler = NULL;
static ckLowLevelAPI::ExtraEventHandler s_extra_event_handler = NULL;

static const char* s_app_name;
static u16 s_framebuffer_width;
static u16 s_framebuffer_height;
static u16 s_sys_flag;
static bool s_is_framebuffer_size_changed;
static bool s_is_fullscreen;
static bool s_is_mouse_visible;

//static Display* s_dpy;
//static Window s_win;
//static GLXContext s_ctx;
//static int s_scr;
//static XF86VidModeModeInfo s_video_mode_info;
//static bool s_is_double_buffered;


/*static void callKeyEventHandler(KeyCode keycode, bool is_down)
{
    KeySym keysym = XKeycodeToKeysym(s_dpy, keycode, 0);
    ckKeyMgr::KeyType key_type = ckKeyMgr::KEY_NONE;

    if (keysym >= XK_0 && keysym <= XK_9)
    {
        key_type = static_cast<ckKeyMgr::KeyType>(ckKeyMgr::KEY_0 + keysym - XK_0);
    }
    else if (keysym >= XK_A && keysym <= XK_Z)
    {
        key_type = static_cast<ckKeyMgr::KeyType>(ckKeyMgr::KEY_A + keysym - XK_A);
    }
    else if (keysym >= XK_a && keysym <= XK_z)
    {
        key_type = static_cast<ckKeyMgr::KeyType>(ckKeyMgr::KEY_A + keysym - XK_a);
    }
    else if (keysym >= XK_F1 && keysym <= XK_F12)
    {
        key_type = static_cast<ckKeyMgr::KeyType>(ckKeyMgr::KEY_F1 + keysym - XK_F1);
    }
    else if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
    {
        key_type = static_cast<ckKeyMgr::KeyType>(ckKeyMgr::KEY_NUMPAD0 + keysym - XK_KP_0);
    }
    else if (keysym >= XK_Home && keysym <= XK_End)
    {
        static const ckKeyMgr::KeyType s_keycode_table[] =
        {
            ckKeyMgr::KEY_HOME, ckKeyMgr::KEY_LEFT, ckKeyMgr::KEY_UP, ckKeyMgr::KEY_RIGHT, //
            ckKeyMgr::KEY_DOWN, ckKeyMgr::KEY_PAGEUP, ckKeyMgr::KEY_PAGEDOWN, ckKeyMgr::KEY_END
        };

        key_type = s_keycode_table[keysym - XK_Home];
    }
    else if (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide)
    {
        static const ckKeyMgr::KeyType s_keycode_table[] =
        {
            ckKeyMgr::KEY_MULTIPLY, ckKeyMgr::KEY_ADD, ckKeyMgr::KEY_NONE, //
            ckKeyMgr::KEY_SUBTRACT, ckKeyMgr::KEY_DECIMAL, ckKeyMgr::KEY_DIVIDE
        };

        key_type = s_keycode_table[keysym - XK_KP_Multiply];
    }
    else if (keysym == XK_BackSpace)
    {
        key_type = ckKeyMgr::KEY_BACKSPACE;
    }
    else if (keysym == XK_Tab)
    {
        key_type = ckKeyMgr::KEY_TAB;
    }
    else if (keysym == XK_Return)
    {
        key_type = ckKeyMgr::KEY_ENTER;
    }
    else if (keysym == XK_Escape)
    {
        key_type = ckKeyMgr::KEY_ESCAPE;
    }
    else if (keysym == XK_space)
    {
        key_type = ckKeyMgr::KEY_SPACE;
    }
    else if (keysym == XK_Insert)
    {
        key_type = ckKeyMgr::KEY_INSERT;
    }
    else if (keysym == XK_Delete)
    {
        key_type = ckKeyMgr::KEY_DELETE;
    }
    else if (keysym == XK_KP_Enter)
    {
        key_type = ckKeyMgr::KEY_SEPARATOR;
    }
    else if (keysym == XK_Shift_L || keysym == XK_Shift_R)
    {
        key_type = ckKeyMgr::KEY_SHIFT;
    }
    else if (keysym == XK_Control_L || keysym == XK_Control_R)
    {
        key_type = ckKeyMgr::KEY_CTRL;
    }
    else if (keysym == XK_Alt_L || keysym == XK_Alt_R)
    {
        key_type = ckKeyMgr::KEY_ALT;
    }

    (*s_key_event_handler)(key_type, is_down);
}*/


static void destroyFramebuffer()
{
    // TODO
}


static bool createFramebuffer(u16 new_width, u16 new_height)
{
    // TODO
    //
    return true;
}


bool ckLowLevelAPI::createApplication(const char* title, u16 width, u16 height, u16 sys_flag)
{
    s_app_name = title;
    s_framebuffer_width = width;
    s_framebuffer_height = height;
    s_sys_flag = sys_flag;
    s_is_framebuffer_size_changed = false;
    s_is_fullscreen = (sys_flag & ckSysMgr::FLAG_FULLSCREEN_START) ? true : false;
    s_is_mouse_visible = true;

    if (!createFramebuffer(width, height))
    {
        return false;
    }

    setupShaderAPI((sys_flag & ckSysMgr::FLAG_DISABLE_SHADER) ? false : true);

    return true;
}


void ckLowLevelAPI::destroyApplication()
{
    destroyFramebuffer();
}


void ckLowLevelAPI::startApplication(bool (*update_func)(void))
{
    while (true)
    {
#if 0
        while (XPending(s_dpy) > 0)
        {
            XEvent event;
            XNextEvent(s_dpy, &event);

            switch (event.type)
            {
            case KeyPress:
                callKeyEventHandler(event.xkey.keycode, true);
                break;

            case KeyRelease:
                callKeyEventHandler(event.xkey.keycode, false);
                break;

            case ButtonPress:
                if (event.xbutton.button == Button1)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_LBUTTON, true);
                }
                else if (event.xbutton.button == Button2)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_MBUTTON, true);
                }
                else if (event.xbutton.button == Button3)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_RBUTTON, true);
                }
                else if (event.xbutton.button == Button4)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_WHEELUP, true);
                }
                else if (event.xbutton.button == Button5)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_WHEELDOWN, true);
                }
                break;

            case ButtonRelease:
                if (event.xbutton.button == Button1)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_LBUTTON, false);
                }
                else if (event.xbutton.button == Button2)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_MBUTTON, false);
                }
                else if (event.xbutton.button == Button3)
                {
                    (*s_key_event_handler)(ckKeyMgr::KEY_RBUTTON, false);
                }
                break;

            case ClientMessage:
                if (*XGetAtomName(s_dpy, event.xclient.message_type) == *"WM_PROTOCOLS")
                {
                    return;
                }
                break;

            default:
                break; // TODO
            }
        }

        Window root, child;
        int root_x, root_y;
        int win_x, win_y;
        unsigned int mask;

        XQueryPointer(s_dpy, s_win, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
        (*s_mouse_event_handler)(static_cast<s16>(win_x), static_cast<s16>(win_y));

        (*update_func)();
#endif
    }
}


u16 ckLowLevelAPI::getFramebufferWidth()
{
    return s_framebuffer_width;
}


u16 ckLowLevelAPI::getFramebufferHeight()
{
    return s_framebuffer_height;
}


void ckLowLevelAPI::updateFramebufferSize()
{
    /*
    Window root;
    int win_x, win_y;
    unsigned int win_width, win_height;
    unsigned int boarder_width;
    unsigned int depth;

    XGetGeometry(s_dpy, s_win, &root, &win_x, &win_y, &win_width, &win_height, &boarder_width, &depth);

    if (win_width != s_framebuffer_width || win_height != s_framebuffer_height)
    {
        s_framebuffer_width = static_cast<u16>(win_width);
        s_framebuffer_height = static_cast<u16>(win_height);
        s_is_framebuffer_size_changed = true;
    }
    else
    {
        s_is_framebuffer_size_changed = false;
    }
    */
}


bool ckLowLevelAPI::isFramebufferSizeChanged()
{
    return s_is_framebuffer_size_changed;
}


void ckLowLevelAPI::swapFramebuffer()
{
    // TODO
}


bool ckLowLevelAPI::isFullScreen()
{
    return s_is_fullscreen;
}


bool ckLowLevelAPI::toggleFullScreen(u16 width, u16 height)
{
    destroyFramebuffer();

    s_is_fullscreen = !s_is_fullscreen;

    return createFramebuffer(width, height);
}


void ckLowLevelAPI::setKeyEventHandler(KeyEventHandler handler)
{
    s_key_event_handler = handler;
}


void ckLowLevelAPI::setMouseEventHandler(MouseEventHandler handler)
{
    s_mouse_event_handler = handler;
}


void ckLowLevelAPI::setExtraEventHandler(ExtraEventHandler handler)
{
    s_extra_event_handler = handler;
}


void ckLowLevelAPI::setMousePos(s16 mouse_x, s16 mouse_y)
{
    // TODO
}


bool ckLowLevelAPI::isMouseVisible()
{
    return s_is_mouse_visible;
}


void ckLowLevelAPI::setMouseVisible(bool is_visible)
{
    if (is_visible != s_is_mouse_visible)
    {
        s_is_mouse_visible = is_visible;

        if (s_is_mouse_visible)
        {
            // TODO
        }
        else
        {
            // TODO
        }
    }
}


u64 ckLowLevelAPI::getUsecTime()
{
    static u64 s_start_time;
    static bool s_is_first = true;

    struct timeval time;
    struct timezone tz;

    if (s_is_first)
    {
        s_is_first = false;

        gettimeofday(&time, &tz);
        s_start_time = time.tv_sec * 1000000 + time.tv_usec;
    }

    gettimeofday(&time, &tz);
    u64 cur_time = time.tv_sec * 1000000 + time.tv_usec;

    return cur_time - s_start_time;
}


void ckLowLevelAPI::sleepUsec(u64 usec)
{
    usleep(usec);
}


void ckLowLevelAPI::exit(s32 status)
{
    ::exit(status);
}


void ckLowLevelAPI::error(const char* msg)
{
    printf(msg);
    printf("\n");

    exit(1);
}


void ckLowLevelAPI::readLittleEndian(void* dest, const void* src, u32 size)
{
    memcpy(dest, src, size);
}


void ckLowLevelAPI::writeLittleEndian(void* dest, const void* src, u32 size)
{
    memcpy(dest, src, size);
}


struct ThreadStartFuncAndUserParam
{
    void (*start_func)(void*);
    void* user_param;
};


static void* threadStartFunc(void* user_param)
{
    ThreadStartFuncAndUserParam* func_and_param = static_cast<ThreadStartFuncAndUserParam*>(user_param);

    func_and_param->start_func(func_and_param->user_param);

    ckLowLevelAPI::free(user_param);

    return NULL;
}


void* ckLowLevelAPI::newThread(void (*start_func)(void*), void* user_param)
{
    static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&s_mutex);

    void* thread_handler = malloc(sizeof(pthread_t));

    ThreadStartFuncAndUserParam* func_and_param = static_cast<ThreadStartFuncAndUserParam*>(malloc(sizeof(ThreadStartFuncAndUserParam)));
    func_and_param->start_func = start_func;
    func_and_param->user_param = user_param;

    if (pthread_create(static_cast<pthread_t*>(thread_handler), NULL, threadStartFunc, func_and_param))
    {
        free(thread_handler);
        free(func_and_param);

        thread_handler = NULL;
    }

    pthread_mutex_unlock(&s_mutex);

    return thread_handler;
}


void ckLowLevelAPI::deleteThread(void* thread_handler)
{
    free(thread_handler);
}


void ckLowLevelAPI::joinThread(void* thread_handler)
{
    pthread_join(*static_cast<pthread_t*>(thread_handler), NULL);
}


void* ckLowLevelAPI::newMutex()
{
    void* mutex_handler = malloc(sizeof(pthread_mutex_t));

    if (pthread_mutex_init(static_cast<pthread_mutex_t*>(mutex_handler), NULL))
    {
        free(mutex_handler);

        return NULL;
    }
    else
    {
        return mutex_handler;
    }
}


void ckLowLevelAPI::deleteMutex(void* mutex_handler)
{
    pthread_mutex_destroy(static_cast<pthread_mutex_t*>(mutex_handler));

    free(mutex_handler);
}


void ckLowLevelAPI::lockMutex(void* mutex_handler)
{
    pthread_mutex_lock(static_cast<pthread_mutex_t*>(mutex_handler));
}


void ckLowLevelAPI::unlockMutex(void* mutex_handler)
{
    pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mutex_handler));
}


void ckLowLevelAPI::setInitialDirectory(s32 argc, char** argv)
{
    if (argc > 0)
    {
        if (chdir(dirname(argv[0])) != 0)
        {
            return; // to avoid gcc warning
        }
    }
}


void ckLowLevelAPI::getWindowsFontDirectory(char* buf, u32 buf_size)
{
    // TODO: Error
}


#endif // CK_ANDROID
