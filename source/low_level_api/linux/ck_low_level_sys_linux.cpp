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


#ifdef PG_LINUX


#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>

#include "pg_low_level_api.h"

#include "pg_sys_all.h" // for pgSysMgr::SysFlag and pgSysMgr::sprintf
#include "pg_key_all.h" // for pgKeyMgr::KeyType


static pgLowLevelAPI::KeyEventHandler s_key_event_handler = NULL;
static pgLowLevelAPI::MouseEventHandler s_mouse_event_handler = NULL;
static pgLowLevelAPI::ExtraEventHandler s_extra_event_handler = NULL;

static const char* s_app_name;
static u16 s_framebuffer_width;
static u16 s_framebuffer_height;
static u16 s_sys_flag;
static bool s_is_framebuffer_size_changed;
static bool s_is_fullscreen;
static bool s_is_mouse_visible;

static Display* s_dpy;
static Window s_win;
static GLXContext s_ctx;
static int s_scr;
static XF86VidModeModeInfo s_video_mode_info;
static bool s_is_double_buffered;


static void callKeyEventHandler(KeyCode keycode, bool is_down)
{
    KeySym keysym = XKeycodeToKeysym(s_dpy, keycode, 0);
    pgKeyMgr::KeyType key_type = pgKeyMgr::KEY_NONE;

    if (keysym >= XK_0 && keysym <= XK_9)
    {
        key_type = static_cast<pgKeyMgr::KeyType>(pgKeyMgr::KEY_0 + keysym - XK_0);
    }
    else if (keysym >= XK_A && keysym <= XK_Z)
    {
        key_type = static_cast<pgKeyMgr::KeyType>(pgKeyMgr::KEY_A + keysym - XK_A);
    }
    else if (keysym >= XK_a && keysym <= XK_z)
    {
        key_type = static_cast<pgKeyMgr::KeyType>(pgKeyMgr::KEY_A + keysym - XK_a);
    }
    else if (keysym >= XK_F1 && keysym <= XK_F12)
    {
        key_type = static_cast<pgKeyMgr::KeyType>(pgKeyMgr::KEY_F1 + keysym - XK_F1);
    }
    else if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
    {
        key_type = static_cast<pgKeyMgr::KeyType>(pgKeyMgr::KEY_NUMPAD0 + keysym - XK_KP_0);
    }
    else if (keysym >= XK_Home && keysym <= XK_End)
    {
        static const pgKeyMgr::KeyType s_keycode_table[] =
        {
            pgKeyMgr::KEY_HOME, pgKeyMgr::KEY_LEFT, pgKeyMgr::KEY_UP, pgKeyMgr::KEY_RIGHT, //
            pgKeyMgr::KEY_DOWN, pgKeyMgr::KEY_PAGEUP, pgKeyMgr::KEY_PAGEDOWN, pgKeyMgr::KEY_END
        };

        key_type = s_keycode_table[keysym - XK_Home];
    }
    else if (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide)
    {
        static const pgKeyMgr::KeyType s_keycode_table[] =
        {
            pgKeyMgr::KEY_MULTIPLY, pgKeyMgr::KEY_ADD, pgKeyMgr::KEY_NONE, //
            pgKeyMgr::KEY_SUBTRACT, pgKeyMgr::KEY_DECIMAL, pgKeyMgr::KEY_DIVIDE
        };

        key_type = s_keycode_table[keysym - XK_KP_Multiply];
    }
    else if (keysym == XK_BackSpace)
    {
        key_type = pgKeyMgr::KEY_BACKSPACE;
    }
    else if (keysym == XK_Tab)
    {
        key_type = pgKeyMgr::KEY_TAB;
    }
    else if (keysym == XK_Return)
    {
        key_type = pgKeyMgr::KEY_ENTER;
    }
    else if (keysym == XK_Escape)
    {
        key_type = pgKeyMgr::KEY_ESCAPE;
    }
    else if (keysym == XK_space)
    {
        key_type = pgKeyMgr::KEY_SPACE;
    }
    else if (keysym == XK_Insert)
    {
        key_type = pgKeyMgr::KEY_INSERT;
    }
    else if (keysym == XK_Delete)
    {
        key_type = pgKeyMgr::KEY_DELETE;
    }
    else if (keysym == XK_KP_Enter)
    {
        key_type = pgKeyMgr::KEY_SEPARATOR;
    }
    else if (keysym == XK_Shift_L || keysym == XK_Shift_R)
    {
        key_type = pgKeyMgr::KEY_SHIFT;
    }
    else if (keysym == XK_Control_L || keysym == XK_Control_R)
    {
        key_type = pgKeyMgr::KEY_CTRL;
    }
    else if (keysym == XK_Alt_L || keysym == XK_Alt_R)
    {
        key_type = pgKeyMgr::KEY_ALT;
    }

    (*s_key_event_handler)(key_type, is_down);
}


static void destroyFramebuffer()
{
    if (s_ctx)
    {
        glXMakeCurrent(s_dpy, None, NULL);
        glXDestroyContext(s_dpy, s_ctx);
        s_ctx = NULL;
    }

    if (s_is_fullscreen)
    {
        XF86VidModeSwitchToMode(s_dpy, s_scr, &s_video_mode_info);
        XF86VidModeSetViewPort(s_dpy, s_scr, 0, 0);
    }

    XCloseDisplay(s_dpy);
}


static bool createFramebuffer(u16 new_width, u16 new_height)
{
    s_dpy = XOpenDisplay(0);
    s_scr = DefaultScreen(s_dpy);

    int video_mode_num;
    XF86VidModeModeInfo** video_mode_info_list;

    XF86VidModeGetAllModeLines(s_dpy, s_scr, &video_mode_num, &video_mode_info_list);
    s_video_mode_info = *video_mode_info_list[0];

    int best_video_mode = 0;

    for (int i = 0; i < video_mode_num; i++)
    {
        if (video_mode_info_list[i]->hdisplay == new_width && video_mode_info_list[i]->vdisplay == new_height)
        {
            best_video_mode = i;
        }
    }

    static int attr_list_double[] =
    {
        GLX_RGBA, GLX_DOUBLEBUFFER, //
        GLX_RED_SIZE, 1, //
        GLX_GREEN_SIZE, 1, //
        GLX_BLUE_SIZE, 1, //
        GLX_ALPHA_SIZE, 1, //
        GLX_DEPTH_SIZE, 1, //
        None
    };

    XVisualInfo* visual_info = glXChooseVisual(s_dpy, s_scr, attr_list_double);

    if (visual_info)
    {
        s_is_double_buffered = true;
    }
    else
    {
        static int attr_list_single[] =
        {
            GLX_RGBA, //
            GLX_RED_SIZE, 1, //
            GLX_GREEN_SIZE, 1, //
            GLX_BLUE_SIZE, 1, //
            GLX_ALPHA_SIZE, 1, //
            GLX_DEPTH_SIZE, 1, //
            None
        };

        visual_info = glXChooseVisual(s_dpy, s_scr, attr_list_single);
        s_is_double_buffered = false;
    }

    s_ctx = glXCreateContext(s_dpy, visual_info, 0, GL_TRUE);

    XSetWindowAttributes win_attr;

    win_attr.colormap = XCreateColormap(s_dpy, RootWindow(s_dpy, visual_info->screen), visual_info->visual, AllocNone);
    win_attr.border_pixel = 0;

    if (s_is_fullscreen)
    {
        XF86VidModeSwitchToMode(s_dpy, s_scr, video_mode_info_list[best_video_mode]);
        XF86VidModeSetViewPort(s_dpy, s_scr, 0, 0);

        int dpy_width = video_mode_info_list[best_video_mode]->hdisplay;
        int dpy_height = video_mode_info_list[best_video_mode]->vdisplay;

        XFree(video_mode_info_list);

        win_attr.override_redirect = true;
        win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;

        s_win = XCreateWindow( //
            s_dpy, RootWindow(s_dpy, visual_info->screen), 0, 0, //
            dpy_width, dpy_height, 0, visual_info->depth, InputOutput, visual_info->visual, //
            CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &win_attr);

        XWarpPointer(s_dpy, None, s_win, 0, 0, 0, 0, 0, 0);
        XMapRaised(s_dpy, s_win);
        XGrabKeyboard(s_dpy, s_win, true, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(s_dpy, s_win, true, ButtonPressMask, GrabModeAsync, GrabModeAsync, s_win, None, CurrentTime);

        if (s_is_mouse_visible == false)
        {
            Cursor cursor;
            XColor xcol;
            Pixmap pixmap;
            char bitmap_bits[1] = /**/ { /**/ 0 /**/ };

            pixmap = XCreateBitmapFromData(s_dpy, s_win, bitmap_bits, 1, 1);
            cursor = XCreatePixmapCursor(s_dpy, pixmap, pixmap, &xcol, &xcol, 0, 0);
            XDefineCursor(s_dpy, s_win, cursor);

            XFreeCursor(s_dpy, cursor);
            XFreePixmap(s_dpy, pixmap);
        }
    }
    else
    {
        win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;

        s_win = XCreateWindow( //
            s_dpy, RootWindow(s_dpy, visual_info->screen), 0, 0, //
            new_width, new_height, 0, visual_info->depth, InputOutput, visual_info->visual, //
            CWBorderPixel | CWColormap | CWEventMask, &win_attr);

        Atom wm_delete = XInternAtom(s_dpy, "WM_DELETE_WINDOW", true);
        XSetWMProtocols(s_dpy, s_win, &wm_delete, 1);
        XSetStandardProperties(s_dpy, s_win, s_app_name, s_app_name, None, NULL, 0, NULL);
        XMapRaised(s_dpy, s_win);

        XStoreName(s_dpy, s_win, s_app_name);
        XSetIconName(s_dpy, s_win, s_app_name);

        XSizeHints size_hints;
        size_hints.flags = USSize;
        size_hints.width = new_width;
        size_hints.height = new_height;

        if (!(s_sys_flag & pgSysMgr::FLAG_VARIABLE_SIZE))
        {
            size_hints.flags |= PMinSize | PMaxSize;
            size_hints.min_width = new_width;
            size_hints.min_height = new_height;
            size_hints.max_width = new_width;
            size_hints.max_height = new_height;
        }

        XSetWMNormalHints(s_dpy, s_win, &size_hints);
        XMapWindow(s_dpy, s_win);
    }

    glXMakeCurrent(s_dpy, s_win, s_ctx);

    Window root;
    int win_x, win_y;
    unsigned int win_width, win_height;
    unsigned int boarder_width;
    unsigned int depth;

    XGetGeometry(s_dpy, s_win, &root, &win_x, &win_y, &win_width, &win_height, &boarder_width, &depth);

    s_framebuffer_width = static_cast<u16>(win_width);
    s_framebuffer_height = static_cast<u16>(win_height);

    return true;
}


bool pgLowLevelAPI::createApplication(const char* title, u16 width, u16 height, u16 sys_flag)
{
    s_app_name = title;
    s_framebuffer_width = width;
    s_framebuffer_height = height;
    s_sys_flag = sys_flag;
    s_is_framebuffer_size_changed = false;
    s_is_fullscreen = (sys_flag & pgSysMgr::FLAG_FULLSCREEN_START) ? true : false;
    s_is_mouse_visible = true;

    if (!createFramebuffer(width, height))
    {
        return false;
    }

    setupShaderAPI((sys_flag & pgSysMgr::FLAG_DISABLE_SHADER) ? false : true);

    return true;
}


void pgLowLevelAPI::destroyApplication()
{
    destroyFramebuffer();
}


void pgLowLevelAPI::startApplication(bool (*update_func)(void))
{
    while (true)
    {
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
                    (*s_key_event_handler)(pgKeyMgr::KEY_LBUTTON, true);
                }
                else if (event.xbutton.button == Button2)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_MBUTTON, true);
                }
                else if (event.xbutton.button == Button3)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_RBUTTON, true);
                }
                else if (event.xbutton.button == Button4)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_WHEELUP, true);
                }
                else if (event.xbutton.button == Button5)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_WHEELDOWN, true);
                }
                break;

            case ButtonRelease:
                if (event.xbutton.button == Button1)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_LBUTTON, false);
                }
                else if (event.xbutton.button == Button2)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_MBUTTON, false);
                }
                else if (event.xbutton.button == Button3)
                {
                    (*s_key_event_handler)(pgKeyMgr::KEY_RBUTTON, false);
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
    }
}


u16 pgLowLevelAPI::getFramebufferWidth()
{
    return s_framebuffer_width;
}


u16 pgLowLevelAPI::getFramebufferHeight()
{
    return s_framebuffer_height;
}


void pgLowLevelAPI::updateFramebufferSize()
{
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
}


bool pgLowLevelAPI::isFramebufferSizeChanged()
{
    return s_is_framebuffer_size_changed;
}


void pgLowLevelAPI::swapFramebuffer()
{
    if (s_is_double_buffered)
    {
        glXSwapBuffers(s_dpy, s_win);
    }
}


bool pgLowLevelAPI::isFullScreen()
{
    return s_is_fullscreen;
}


bool pgLowLevelAPI::toggleFullScreen(u16 width, u16 height)
{
    destroyFramebuffer();

    s_is_fullscreen = !s_is_fullscreen;

    return createFramebuffer(width, height);
}


void pgLowLevelAPI::setKeyEventHandler(KeyEventHandler handler)
{
    s_key_event_handler = handler;
}


void pgLowLevelAPI::setMouseEventHandler(MouseEventHandler handler)
{
    s_mouse_event_handler = handler;
}


void pgLowLevelAPI::setExtraEventHandler(ExtraEventHandler handler)
{
    s_extra_event_handler = handler;
}


void pgLowLevelAPI::setMousePos(s16 mouse_x, s16 mouse_y)
{
    /*
        Window root;
        int win_x, win_y;
        unsigned int win_width, win_height;
        unsigned int boarder_width;
        unsigned int depth;

        XGetGeometry(s_dpy, s_win, &root, &win_x, &win_y, &win_width, &win_height, &boarder_width, &depth);

        x =  mouse_x + win_width  / 2;
        y = -mouse_y + win_height / 2;
    */

    XWarpPointer(s_dpy, None, s_win, 0, 0, 0, 0, mouse_x, mouse_y);
}


bool pgLowLevelAPI::isMouseVisible()
{
    return s_is_mouse_visible;
}


void pgLowLevelAPI::setMouseVisible(bool is_visible)
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
            Cursor cursor;
            XColor xcol;
            Pixmap pixmap;
            char bitmap_bits[1] = /**/ { /**/ 0 /**/ };

            pixmap = XCreateBitmapFromData(s_dpy, s_win, bitmap_bits, 1, 1);
            cursor = XCreatePixmapCursor(s_dpy, pixmap, pixmap, &xcol, &xcol, 0, 0);
            XDefineCursor(s_dpy, s_win, cursor);

            XFreeCursor(s_dpy, cursor);
            XFreePixmap(s_dpy, pixmap);
        }
    }
}


u64 pgLowLevelAPI::getUsecTime()
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


void pgLowLevelAPI::sleepUsec(u64 usec)
{
    usleep(usec);
}


void pgLowLevelAPI::exit(s32 status)
{
    ::exit(status);
}


void pgLowLevelAPI::error(const char* msg)
{
    printf(msg);
    printf("\n");

    exit(1);
}


void pgLowLevelAPI::readLittleEndian(void* dest, const void* src, u32 size)
{
    memcpy(dest, src, size);
}


void pgLowLevelAPI::writeLittleEndian(void* dest, const void* src, u32 size)
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

    pgLowLevelAPI::free(user_param);

    return NULL;
}


void* pgLowLevelAPI::newThread(void (*start_func)(void*), void* user_param)
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


void pgLowLevelAPI::deleteThread(void* thread_handler)
{
    free(thread_handler);
}


void pgLowLevelAPI::joinThread(void* thread_handler)
{
    pthread_join(*static_cast<pthread_t*>(thread_handler), NULL);
}


void* pgLowLevelAPI::newMutex()
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


void pgLowLevelAPI::deleteMutex(void* mutex_handler)
{
    pthread_mutex_destroy(static_cast<pthread_mutex_t*>(mutex_handler));

    free(mutex_handler);
}


void pgLowLevelAPI::lockMutex(void* mutex_handler)
{
    pthread_mutex_lock(static_cast<pthread_mutex_t*>(mutex_handler));
}


void pgLowLevelAPI::unlockMutex(void* mutex_handler)
{
    pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mutex_handler));
}


void pgLowLevelAPI::setInitialDirectory(s32 argc, char** argv)
{
    if (argc > 0)
    {
        if (chdir(dirname(argv[0])) != 0)
        {
            return; // to avoid gcc warning
        }
    }
}


void pgLowLevelAPI::getWindowsFontDirectory(char* buf, u32 buf_size)
{
    // TODO: Error
}


#endif // PG_LINUX