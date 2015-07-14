#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <Windows.h>

static bool running = false;

unsigned char chip8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

struct opcode_params_t
{
  unsigned short p,x,y,kk,nnn,l;
};

#define CHIP8_GFX_WIDTH             64
#define CHIP8_GFX_HEIGHT            32
#define CHIP8_MEMORY_SIZE           4096
#define CHIP8_REGISTER_COUNT        16
#define CHIP8_STACK_SIZE            16
#define CHIP8_KEY_COUNT             16
#define CHIP8_MEMORY_FONTSET_OFFSET 0x000
#define CHIP8_MEMORY_FONT_OFFSET    0x050
#define CHIP8_MEMORY_PROGRAM_OFFSET 0x200

class chip8{
  private:
    unsigned char delay_timer, sound_timer;
    unsigned char memory[CHIP8_MEMORY_SIZE];
    unsigned char V[CHIP8_REGISTER_COUNT];
    unsigned short I,pc,sp,opcode;
    unsigned short stack[CHIP8_STACK_SIZE];

    void draw(opcode_params_t opcodeParams)
    {
      unsigned short row;
      V[0xF] = 0;
      for(int yline = 0; yline < opcodeParams.l; yline++)
      {
        row = memory[I + yline];
        for(int xline = 0; xline < 8; xline++)
        {
          if((row & (0x80 >> xline)) != 0)
          {
            if(gfx[(V[opcodeParams.x] + xline + ((V[opcodeParams.y] + yline) * CHIP8_GFX_WIDTH))] == 1) V[0xF] = 1;
            gfx[V[opcodeParams.x] + xline + ((V[opcodeParams.y] + yline) * CHIP8_GFX_WIDTH)] ^= 1;
          }
        }
      }
    }

    int handleOpcode(unsigned short a_opcode)
    {
      opcode_params_t op;

      op.p   = (opcode & 0xF000) >> 12;
      op.x   = (opcode & 0x0F00) >> 8;
      op.y   = (opcode & 0x00F0) >> 4;
      op.kk  = opcode & 0x00FF;
      op.nnn = opcode & 0x0FFF;
      op.l   = opcode & 0x000F;

      #define t(test,ops) if(test) { ops; }
      t(op.p==0x0 && op.nnn==0xE0, for(int i = 0; i < CHIP8_GFX_WIDTH*CHIP8_GFX_HEIGHT; i++) gfx[i] = 0);
      t(op.p==0x0 && op.nnn==0xEE, pc=stack[sp--]                                                      );
      t(op.p==0x1                , pc= op.nnn                                                          );
      t(op.p==0x2                , stack[++sp] = pc; pc = op.nnn                                       );
      t(op.p==0x3                , if(op.kk == V[op.x])   pc += 2                                      );
      t(op.p==0x4                , if(op.kk != V[op.x])   pc += 2                                      );
      t(op.p==0x5 && op.l==0x0   , if(V[op.y] == V[op.x]) pc += 2                                      );
      t(op.p==0x6                , V[op.x] =  op.kk                                                    );
      t(op.p==0x7                , V[op.x] += op.kk                                                    );
      t(op.p==0x8 && op.l==0x1   , V[op.x] |= V[op.y]                                                  );
      t(op.p==0x8 && op.l==0x2   , V[op.x] &= V[op.y]                                                  );
      t(op.p==0x8 && op.l==0x3   , V[op.x] ^= V[op.y]                                                  );
      t(op.p==0x8 && op.l==0x4   , if(V[op.y] > (0xFF - V[op.x])) V[0xF] = 1;
                                     else V[0xF] = 0; V[op.x] += V[op.y]                               );
      t(op.p==0x8 && op.l==0x5   , if(V[op.y] > V[op.x]) V[0xF] = 0; else V[0xF] = 1;V[op.x] -= V[op.y]);
      t(op.p==0x8 && op.l==0x6   , V[0xF] = V[op.y]  & 1;  V[op.x] = V[op.y] >> 1                      );
      t(op.p==0x8 && op.l==0x7   , op.l = V[op.y]-V[op.x]; V[0xF] = !(op.l>>8); V[op.x] = op.l         );
      t(op.p==0x8 && op.l==0xE   , V[0xF] = V[op.y] >> 7;  V[op.x] = V[op.y] << 1                      );
      t(op.p==0x8 && op.l==0x0   , V[op.x] = V[op.y]                                                   );
      t(op.p==0x9 && op.l==0x0   , if(V[op.y] != V[op.x]) pc += 2;                                     );
      t(op.p==0xA                , I = op.nnn                                                          );
      t(op.p==0xB                , pc= op.nnn + V[0]                                                   );
      t(op.p==0xC                , V[op.x] = (rand() % 256) & op.kk                                    );
      t(op.p==0xD                , draw(op)                                                            );
      t(op.p==0xE && op.kk==0x9E , if( key[V[op.x]] == 1) pc += 2                                      );
      t(op.p==0xE && op.kk==0xA1 , if( key[V[op.x]] == 0) pc += 2                                      );
      t(op.p==0xF && op.kk==0x0A , for(int i = 0; i < CHIP8_KEY_COUNT; i++)
        if(key[i]) {pc += 2; V[op.x] = i;} else pc -= 2                                                );
      t(op.p==0xF && op.kk==0x29 , I = 5*V[op.x];                                                      );
      t(op.p==0xF && op.kk==0x07 , V[op.x] = delay_timer                                               );
      t(op.p==0xF && op.kk==0x15 , delay_timer = V[op.x]                                               );
      t(op.p==0xF && op.kk==0x18 , sound_timer = V[op.x]                                               );
      t(op.p==0xF && op.kk==0x1E , I += V[op.x];                                                       ); //IF wont work, repair
      t(op.p==0xF && op.kk==0x33 , memory[I    ] = (V[op.x] / 100);
                                   memory[I + 1] = (V[op.x] / 10 )%10;
                                   memory[I + 2] = (V[op.x] % 100)%10                                  );
      t(op.p==0xF && op.kk==0x55 , for(int i = 0; i <= op.x; i++) memory[I++] = V[i];                  );
      t(op.p==0xF && op.kk==0x65 , for(int i = 0; i <= op.x; i++) V[i] = memory[I++];                  );

    }

  public:
    unsigned char key[CHIP8_KEY_COUNT];
    unsigned char gfx[CHIP8_GFX_WIDTH*CHIP8_GFX_HEIGHT];

    chip8()
    {
      pc     = CHIP8_MEMORY_PROGRAM_OFFSET;
      opcode = 0;
      I      = 0;
      sp     = 0;  

      for(int i = 0; i < CHIP8_GFX_WIDTH*CHIP8_GFX_HEIGHT; i++)   gfx[i] = 0;
      for(int i = 0; i < CHIP8_STACK_SIZE; i++)                   stack[i] = 0;
      for(int i = 0; i < CHIP8_REGISTER_COUNT; i++)               V[i] = 0;
      for(int i = 0; i < CHIP8_MEMORY_SIZE; i++)                  memory[i] = 0;
      for(int i = 0; i < CHIP8_KEY_COUNT; i++)                    key[i] = char(0);

      for(int i = 0; i < 80; ++i)   memory[i] = chip8_fontset[i];

      delay_timer = 0;
      sound_timer = 0; 
    }

    int loadProgram(char* path)
    {
      FILE * pFile;
      pFile = fopen(path,"rb"); 
      if (pFile==NULL) return 1;
      fread(memory+CHIP8_MEMORY_PROGRAM_OFFSET,sizeof(char),512,pFile);
      fclose(pFile);
    }

    void updateCycle()
    {
      opcode = memory[pc] << 8 | memory[pc+1];
      pc += 2;

      printf("Opcode number: %X\n", opcode);
      
      handleOpcode(opcode);

      if(delay_timer > 0) --delay_timer;
      if(sound_timer > 0){
        if(sound_timer == 1) Beep(1000,100);
        --sound_timer;
      }
    }
};

void handleEvent(SDL_Event event,chip8* chip)
{
  while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
        case SDL_KEYDOWN:
          switch(event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
              running = false;
            break;

            case SDLK_x:
              chip->key[ 0] = 1;
            break;

            case SDLK_1:
              chip->key[ 1] = 1;
            break;

            case SDLK_2:
              chip->key[ 2] = 1;
            break;
            
            case SDLK_3:
              chip->key[ 3] = 1;
            break;
            
            case SDLK_q:
              chip->key[ 4] = 1;
            break;
            
            case SDLK_w:
              chip->key[ 5] = 1;
            break;
            
            case SDLK_e:
              chip->key[ 6] = 1;
            break;
            
            case SDLK_a:
              chip->key[ 7] = 1;
            break;
            
            case SDLK_s:
              chip->key[ 8] = 1;
            break;
            
            case SDLK_d:
              chip->key[ 9] = 1;
            break;
            
            case SDLK_z:
              chip->key[10] = 1;
            break;
            
            case SDLK_c:
              chip->key[11] = 1;
            break;
            
            case SDLK_4:
              chip->key[12] = 1;
            break;
            
            case SDLK_r:
              chip->key[13] = 1;
            break;

            case SDLK_f:
              chip->key[14] = 1;
            break;
            
            case SDLK_v:
              chip->key[15] = 1;
            break;
          }
        break;
        
        case SDL_KEYUP:
          switch(event.key.keysym.sym)
          {
            case SDLK_x:
              chip->key[ 0] = 0;
            break;

            case SDLK_1:
              chip->key[ 1] = 0;
            break;

            case SDLK_2:
              chip->key[ 2] = 0;
            break;
            
            case SDLK_3:
              chip->key[ 3] = 0;
            break;
            
            case SDLK_q:
              chip->key[ 4] = 0;
            break;
            
            case SDLK_w:
              chip->key[ 5] = 0;
            break;
            
            case SDLK_e:
              chip->key[ 6] = 0;
            break;
            
            case SDLK_a:
              chip->key[ 7] = 0;
            break;
            
            case SDLK_s:
              chip->key[ 8] = 0;
            break;
            
            case SDLK_d:
              chip->key[ 9] = 0;
            break;
            
            case SDLK_z:
              chip->key[10] = 0;
            break;
            
            case SDLK_c:
              chip->key[11] = 0;
            break;
            
            case SDLK_4:
              chip->key[12] = 0;
            break;
            
            case SDLK_r:
              chip->key[13] = 0;
            break;

            case SDLK_f:
              chip->key[14] = 0;
            break;
            
            case SDLK_v:
              chip->key[15] = 0;
            break;
          }
        break;

        case SDL_QUIT:
          running = false;
        break;
      }
    }

}

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768
#define GFX_COLOR1    0x00009900
#define GFX_COLOR2    0x6699FF00

char* getFileName()
{
  //TODO: choosing game to play - program to load
}

int main(int argc, char* argv[])
{
  chip8 chip;
  SDL_Window* win;
  SDL_Renderer* ren;
  SDL_Event event;
  SDL_Surface *gfx_surface;
  SDL_Texture* ren_texture;
  if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    std::cerr << "Failed to init SDL2." << std::endl;
  }
  chip.loadProgram(argv[1]);
  win = SDL_CreateWindow("DuChip8",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WINDOW_WIDTH,WINDOW_HEIGHT,0);
  ren = SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);

  gfx_surface = SDL_CreateRGBSurface(0,CHIP8_GFX_WIDTH,CHIP8_GFX_HEIGHT,32,0,0,0,0);

  ren_texture = SDL_CreateTexture(ren,gfx_surface->format->format,SDL_TEXTUREACCESS_STREAMING,CHIP8_GFX_WIDTH,CHIP8_GFX_HEIGHT);

  running = true;
  int* pix =(int*) gfx_surface->pixels;
  void* mPixels;
  int mPitch;
  while(running)
  {
    SDL_RenderClear(ren);
    handleEvent(event,&chip);
    chip.updateCycle();

    for(int i = 0; i < CHIP8_GFX_WIDTH*CHIP8_GFX_HEIGHT; i++) if(chip.gfx[i]) pix[i] = GFX_COLOR1; else pix[i] = GFX_COLOR2;
    
    SDL_LockTexture(ren_texture,NULL,&mPixels,&mPitch);
    memcpy(mPixels,gfx_surface->pixels,gfx_surface->pitch * gfx_surface->h);
    SDL_UnlockTexture(ren_texture);

    SDL_RenderCopy(ren, ren_texture, NULL, NULL);
    SDL_RenderPresent(ren);
    SDL_Delay(1000 / 180);
  }

  SDL_DestroyTexture(ren_texture);
  SDL_FreeSurface(gfx_surface);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
