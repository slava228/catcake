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


#ifdef PG_IPHONE


#import "pg_iphone_bridge.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>


void teardown()
{
}

void sig_handler(int ignored)
{
    teardown();
    exit(0);
}

@interface OpenGLView : UIView
{
    NSTimer* timer;
    UIApplication* m_app;
    CGPoint m_mouse_pos;
    CGRect m_window_rect;

    CGPoint m_inner_mouse_pos; // TODO
    CGPoint m_outer_mouse_pos; // TODO

    @private

    GLint backingWidth;
    GLint backingHeight;

    EAGLContext *context;

    GLuint viewRenderbuffer, viewFramebuffer;

    GLuint depthRenderbuffer;

    BOOL bufferClear;
}

- (id)initGLES;
- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;
-(void)drawView;

@end

@implementation OpenGLView


+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithCoder:(NSCoder*)coder
{
    if((self = [super initWithCoder:coder]))
    {
        self = [self initGLES];
    }
    return self;
}

-(id)initGLES
{
    CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;

    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];

    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    if(!context || ![EAGLContext setCurrentContext:context] || ![self createFramebuffer])
    {
        [self release];
        return nil;
    }

    if (timer) {
        [timer invalidate];
    }
    timer =
    [
        NSTimer scheduledTimerWithTimeInterval:1.0 / 30.0
        target:self
        selector:@selector(FireTimer:)
        userInfo:nil
        repeats:YES
    ];

    return self;
}

-(void)layoutSubviews
{
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self createFramebuffer];
    [self drawView];
}

- (BOOL)createFramebuffer
{
    glGenFramebuffersOES(1, &viewFramebuffer);
    glGenRenderbuffersOES(1, &viewRenderbuffer);

    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);

    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)self.layer];
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);

    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);

    glGenRenderbuffersOES(1, &depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);

    if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
    {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }

    return YES;
}

- (void)destroyFramebuffer
{
    glDeleteFramebuffersOES(1, &viewFramebuffer);
    viewFramebuffer = 0;
    glDeleteRenderbuffersOES(1, &viewRenderbuffer);
    viewRenderbuffer = 0;

    if(depthRenderbuffer)
    {
        glDeleteRenderbuffersOES(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
}

- (void)drawView
{
    [EAGLContext setCurrentContext:context];

    if (!bufferClear) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        bufferClear = YES;
    }


    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);

    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];

    GLenum err = glGetError();
    if(err)
        NSLog(@"%x error", err);
}

- (void)dealloc
{
    if([EAGLContext currentContext] == context)
    {
        [EAGLContext setCurrentContext:nil];
    }

    [context release];
    context = nil;

    [super dealloc];
}

- (id)initWithFrame:(CGRect)rect
{
    m_window_rect = rect;

    self = [super initWithFrame:rect];
    if(self != nil)
    {
        self = [self initGLES];
    }
    return self;
}

- (void)drawRect:(CGRect)rect
{
    return [super drawRect:rect];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
{
    NSSet *allTouches = [event allTouches];
    UITouch *touch;

    int num = [allTouches count];
    if (num >= 2) num = 2;
    switch (num) {
    case 2:
        touch = [[allTouches allObjects] objectAtIndex:1];
        m_outer_mouse_pos = [touch locationInView:self];
        pgIPhoneBridge::setMouseState(1, true);
    case 1:
        touch = [[allTouches allObjects] objectAtIndex:0];
        m_mouse_pos = [touch locationInView:self];
        m_inner_mouse_pos = m_mouse_pos;
        pgIPhoneBridge::setMouseState(0, true);
        break;
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
{
    NSSet *allTouches = [event allTouches];
    UITouch *touch;
    int num = [allTouches count];
    if (num >= 2) num = 2;

    switch (num) {
    case 2:
        touch = [[allTouches allObjects] objectAtIndex:1];
        m_outer_mouse_pos = [touch locationInView:self];
        pgIPhoneBridge::setMouseState(1, true);
    case 1:
        touch = [[allTouches allObjects] objectAtIndex:0];
        m_mouse_pos = [touch locationInView:self];
        m_inner_mouse_pos = m_mouse_pos;
        pgIPhoneBridge::setMouseState(0, true);
        break;
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
{
    NSSet *allTouches = [event allTouches];
    UITouch *touch;
    int num = [allTouches count];
    int end_num = [touches count];
    int i, j;
    BOOL found;
    int out = 0;

    for (i = 0; i < num && out < 2; i++) {
        found = FALSE;
        for (j = 0; j < end_num && (found != TRUE); j++) {
            if ([[allTouches allObjects] objectAtIndex:i] == [[touches allObjects] objectAtIndex:j]) {
                found = TRUE;
            }
        }
        if (found != TRUE) {
            if (out == 0) {
                touch = [[allTouches allObjects] objectAtIndex:i];
                m_mouse_pos = [touch locationInView:self];
                m_inner_mouse_pos = m_mouse_pos;
                pgIPhoneBridge::setMouseState(0, true);
            } else if (out == 1) {
                touch = [[allTouches allObjects] objectAtIndex:i];
                m_outer_mouse_pos = [touch locationInView:self];
                pgIPhoneBridge::setMouseState(1, true);
            }
            out++;
        }
    }


    switch (out) {
    case 1:
        pgIPhoneBridge::setMouseState(1, false);
        break;
    case 0:
        pgIPhoneBridge::setMouseState(0, false);
        pgIPhoneBridge::setMouseState(1, false);
        break;
    }
}

- (void)touchesCanceled:(NSSet *)touches withEvent:(UIEvent *)event;
{
}

- (void)SetApplication:(UIApplication*)app
{
    m_app = app;
}

- (void)FireTimer:(NSTimer*)unused
{
    [self drawView];

    pgIPhoneBridge::updateApplication();
}

- (CGPoint&)GetMousePos;
{
    return m_mouse_pos;
}

- (CGPoint&)GetInnerMousePos;
{
    return m_inner_mouse_pos;
}

- (CGPoint&)GetOuterMousePos;
{
    return m_outer_mouse_pos;
}

@end

@interface PogolynApp : UIApplication
{
    OpenGLView* m_view;
    int m_orientation;
    float m_accel_x, m_accel_y, m_accel_z;
}

@end

PogolynApp* g_app;

@implementation PogolynApp

- (void)applicationDidFinishLaunching:(id)unused
{
    signal(SIGINT, &sig_handler);

    CGRect rect = [[UIScreen mainScreen] bounds];
    UIWindow *window = [[UIWindow alloc] initWithFrame:rect];

    m_view = [[OpenGLView alloc] initWithFrame:rect];
    m_view.multipleTouchEnabled = YES;
    [window addSubview:m_view];

    g_app = self;

    [window makeKeyAndVisible];

    m_orientation = 0;
    m_accel_x = m_accel_y = m_accel_z = 0.0f;

    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(didRotate:)
        name:@"UIDeviceOrientationDidChangeNotification"
        object:nil];
}

- (void)didRotate:(NSNotification *)notification
{
    m_orientation = [[UIDevice currentDevice] orientation];
}

- (void)acceleratedInX:(float)xx Y:(float)yy Z:(float)zz;
{
    m_accel_x = xx;
    m_accel_y = yy;
    m_accel_z = zz;
}

/*
- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    [self acceleratedInX:acceleration.x Y:acceleration.y Z:acceleration.z];
}
*/

- (CGPoint&)GetMousePos;
{
    return [m_view GetMousePos];
}

- (CGPoint&)GetInnerMousePos;
{
    return [m_view GetInnerMousePos];
}

- (CGPoint&)GetOuterMousePos;
{
    return [m_view GetOuterMousePos];
}

- (int)GetDeviceOrientation;
{
    return m_orientation;
}

- (float)GetAcceleraionX;
{
    return m_accel_x;
}

- (float)GetAcceleraionY;
{
    return m_accel_y;
}

- (float)GetAcceleraionZ;
{
    return m_accel_z;
}


@end


bool pgIPhoneBridge::createFramebuffer()
{
    return true;
}


void pgIPhoneBridge::destroyFramebuffer() {}


void pgIPhoneBridge::swapFramebuffer()
{
}


void pgIPhoneBridge::startApplication()
{
    [[NSAutoreleasePool alloc] init];

    UIApplicationMain(0, NULL, @"PogolynApp", @"PogolynApp");
}


int pgIPhoneBridge::getFramebufferWidth()
{
    return 320;
}


int pgIPhoneBridge::getFramebufferHeight()
{
    return 480;
}


void pgIPhoneBridge::getMousePos(int* x, int* y)
{
    CGPoint& pos = [g_app GetMousePos];

    *x = pos.x;
    *y = pos.y;
}


void pgIPhoneBridge::getInnerMousePos(int* x, int* y)
{
    CGPoint& pos = [g_app GetInnerMousePos];

    *x = pos.x;
    *y = pos.y;
}


void pgIPhoneBridge::getOuterMousePos(int* x, int* y)
{
    CGPoint& pos = [g_app GetOuterMousePos];

    *x = pos.x;
    *y = pos.y;
}


int pgIPhoneBridge::getDeviceOrientation()
{
    return [g_app GetDeviceOrientation];
}


void pgIPhoneBridge::getAcceleraion(float* x, float* y, float* z)
{
    *x = [g_app GetAcceleraionX];
    *y = [g_app GetAcceleraionY];
    *z = [g_app GetAcceleraionZ];
}


#endif // PG_IPHONE
