#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>

typedef void (*PFN_CLEARCOLOR)(GLfloat,GLfloat,GLfloat,GLfloat);
typedef void (*PFN_CLEAR)(GLbitfield);

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_GLContext context;
    SDL_Event event;
    bool running = true;
    bool fullscreen = false;
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
    GLfloat angle = 0.0f;
    PFN_CLEARCOLOR pClearColor;
    PFN_CLEAR pClear;

    if(argc > 1 && SDL_strcmp(argv[1],"fullscreen")==0){
        fullscreen = true;
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    printf("Creating %s MiniGL window 640x480...\n",
           fullscreen ? "FULLSCREEN" : "windowed");

    window = SDL_CreateWindow("SDL3 AmigaOS3 MiniGL Test",640,480,flags);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n",SDL_GetError());
        SDL_Quit();
        return 1;
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("SDL_GL_CreateContext failed: %s\n",SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if(!SDL_GL_MakeCurrent(window,context)){
        printf("SDL_GL_MakeCurrent failed: %s\n",SDL_GetError());
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* Validate the new fixed MiniGL proc table with real calls. */
    pClearColor=(PFN_CLEARCOLOR)SDL_GL_GetProcAddress("glClearColor");
    pClear=(PFN_CLEAR)SDL_GL_GetProcAddress("glClear");
    printf("GetProcAddress glClearColor: %s\n",pClearColor ? "OK" : "FAIL");
    printf("GetProcAddress glClear:      %s\n",pClear ? "OK" : "FAIL");
    printf("GetProcAddress ARB multitex: %s\n",
           SDL_GL_GetProcAddress("glActiveTextureARB") ? "OK" : "FAIL");
    printf("GetProcAddress core alias:   %s\n",
           SDL_GL_GetProcAddress("glActiveTexture") ? "OK" : "FAIL");

    if(!pClearColor || !pClear){
        printf("GetProcAddress test failed: %s\n",SDL_GetError());
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glViewport(0,0,640,480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.3333,1.3333,-1.0,1.0,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_EVENT_QUIT ||
               (event.type==SDL_EVENT_KEY_DOWN && event.key.key==SDLK_ESCAPE)){
                running=false;
            }
        }

        /* Use function pointers for two calls to prove dispatch works. */
        pClearColor(0.08f,0.08f,0.14f,1.0f);
        pClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();
        glRotatef(angle,0.0f,0.0f,1.0f);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f,0.1f,0.1f);
        glVertex2f(0.0f,0.75f);
        glColor3f(0.1f,1.0f,0.1f);
        glVertex2f(-0.75f,-0.65f);
        glColor3f(0.1f,0.3f,1.0f);
        glVertex2f(0.75f,-0.65f);
        glEnd();

        SDL_GL_SwapWindow(window);
        angle+=1.0f;
        if(angle>=360.0f)angle-=360.0f;
        SDL_Delay(16);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
