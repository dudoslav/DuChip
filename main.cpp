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

    int handle0(opcode_params_t opcodeParams)
    {
      switch(opcodeParams.kk)
      {
        case 0x00E0:
          for(int i = 0; i < CHIP8_GFX_WIDTH*CHIP8_GFX_HEIGHT; i++) gfx[i] = 0;
          pc += 2;
        break;

        case 0x00EE:
          pc = stack[sp];
          --sp;
          pc += 2;
        break;
      }
      return 0;
    }

    int handle8(opcode_params_t opcodeParams)
    {
      switch(opcodeParams.l)
      {
        case 0x0000:
          V[opcodeParams.x] = V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0001:
          V[opcodeParams.x] = V[opcodeParams.x] | V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0002:
          V[opcodeParams.x] = V[opcodeParams.x] & V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0003:
          V[opcodeParams.x] = V[opcodeParams.x] ^ V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0004:
          if(V[opcodeParams.y] > (0xFF - V[opcodeParams.x])) V[0xF] = 1; else V[0xF] = 0;
          V[opcodeParams.x] += V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0005:
          if(V[opcodeParams.y] > V[opcodeParams.x]) V[0xF] = 0; else V[0xF] = 1;
          V[opcodeParams.x] -= V[opcodeParams.y];
          pc += 2;
        break;
        
        case 0x0006:
          if(V[opcodeParams.x] & 0x0001) V[0xF] = 1; else V[0xF] = 0;
          V[opcodeParams.x] / 2;
          pc += 2;
        break;
        
        case 0x0007:
          if(V[opcodeParams.y] > V[opcodeParams.x]) V[0xF] = 1; else V[0xF] = 0;
          V[opcodeParams.x] = V[opcodeParams.y] - V[opcodeParams.x];
          pc += 2;
        break;
        
        case 0x000E:
          if((V[opcodeParams.x] & 0x8000) == 0x8000) V[0xF] = 1; else V[0xF] = 0;
          V[opcodeParams.x] * 2;
          pc += 2;
        break;
      }
      return 0;
    }

    int handleE(opcode_params_t opcodeParams)
    {
      switch(opcodeParams.kk)
      {
        case 0x009E:
          if(key[V[opcodeParams.x]]) pc += 2;
          pc += 2;
        break;

        case 0x00A1:
          if(key[V[opcodeParams.x]] == 0) pc += 2;
          pc += 2;
        break;
      }  
      return 0;
    }

    int handleF(opcode_params_t opcodeParams)
    {
      bool loop = true;
      switch(opcodeParams.kk)
      {
        case 0x0007:
          V[opcodeParams.x] = delay_timer;
          pc += 2;
        break;
        
        case 0x000A:
          for(int i = 0; i < CHIP8_KEY_COUNT; i++) if(key[i]) pc += 2; 
        break;
        
        case 0x0015:
          delay_timer = V[opcodeParams.x];
          pc += 2;
        break;
        
        case 0x0018:
          sound_timer = V[opcodeParams.x];
          pc += 2;
        break;
        
        case 0x001E:
          I += V[opcodeParams.x];
          pc += 2;
        break;
        
        case 0x0029:
          I = 5*V[opcodeParams.x];
          pc += 2;
        break;
        
        case 0x0033:
          memory[I]     = V[opcodeParams.x] / 100;
          memory[I + 1] = (V[opcodeParams.x] / 10) % 10;
          memory[I + 2] = (V[opcodeParams.x] % 100) % 10;
          pc += 2;
        break;
        
        case 0x0055:
          for(int i = 0; i < V[opcodeParams.x]; i++) memory[I+i] = V[i];
          pc += 2;
        break;
        
        case 0x0065:
          for(int i = 0; i < V[opcodeParams.x]; i++) V[i] = memory[I+i];
          pc += 2;
        break;
      }
      return 0;
    }

    int handleOpcode(unsigned short a_opcode)
    {
      opcode_params_t opcodeParams;
      
      opcodeParams.p      = opcode & 0xF000;
      opcodeParams.x      = (opcode & 0x0F00) >> 8;
      opcodeParams.y      = (opcode & 0x00F0) >> 4;
      opcodeParams.kk     = opcode & 0x00FF;
      opcodeParams.nnn    = opcode & 0x0FFF;
      opcodeParams.l      = opcode & 0x000F;

      switch(opcodeParams.p)
      {
        case 0x0000:
          handle0(opcodeParams);
        break;

        case 0x1000:
          pc = opcodeParams.nnn;
        break;
        
        case 0x2000:
          ++sp;
          stack[sp] = pc;
          pc = opcodeParams.nnn;
        break;
        
        case 0x3000:
          if(V[opcodeParams.x] == opcodeParams.kk) pc += 2;
          pc += 2;
        break;
        
        case 0x4000:
          if(V[opcodeParams.x] != opcodeParams.kk) pc += 2;
          pc += 2;
        break;
        
        case 0x5000:
          if(V[opcodeParams.x] == V[opcodeParams.y]) pc += 2;
          pc += 2;
        break;

        case 0x6000:
          V[opcodeParams.x] = opcodeParams.kk;
          pc += 2;
        break;

        case 0x7000:
          V[opcodeParams.x] += opcodeParams.kk;
          pc += 2;
        break;
        
        case 0x8000:
          handle8(opcodeParams);
        break;
        
        case 0x9000:
          if(V[opcodeParams.x] != V[opcodeParams.y]) pc += 2;
          pc += 2;
        break;
        
        case 0xA000:
          I = opcodeParams.nnn;
          pc += 2;
        break;
        
        case 0xB000:
          pc = opcodeParams.nnn + V[0x0];
        break;
        
        case 0xC000:
          V[opcodeParams.x] = (rand() % 256) & opcodeParams.kk;
          pc += 2;
        break;
        
        case 0xD000:
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
          pc += 2;
        break;
        
        case 0xE000:
          handleE(opcodeParams);
        break;
        
        case 0xF000:
          handleF(opcodeParams);
        break;
      }

      return 0;
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
