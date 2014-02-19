#include "player.h"

#define CONTINUE 1
#define END 0

static int  decode_rtp2h264(struct player_context * ctx,char * rtp_buf,int len){
  nalu_header_t * nalu_header;
  fu_header_t   * fu_header;
  nalu_header = (nalu_header_t *)&rtp_buf[12];
  ctx->buf_len += len-14;
  assert(ctx->buf_len < 65536);
  memcpy(ctx->buf_pos,&rtp_buf[14],len-14);
  ctx->buf_pos += len-14;
  if (nalu_header->type == 28) { /* FU-A */
    fu_header = (fu_header_t *)&rtp_buf[13];
    if (fu_header->e == 1) { /* end of fu-a */
      return END;
    } else if (fu_header->s == 1) { /* start of fu-a */
      return CONTINUE;
    } else {
      return CONTINUE;
    }
  } else { /* nalu */
    return END;
  }
}

static int do_play(struct player_context * ctx){
  int frame_finished;
  AVFrame * frame;
  char vedio_data[65536];
  char * ptr;
  AVPacket pack;
  av_init_packet(&pack);
  pack.data = ctx->buf;
  pack.size = ctx->buf_len;
  ptr = vedio_data;
  frame = av_frame_alloc();
  int len = avcodec_decode_video2(ctx->av_ctx, frame, &frame_finished,&pack);
  if(frame_finished){
    int width,height;
    int i;
    width = ctx->av_ctx->width;
    height = ctx->av_ctx->height;
    //copy data y
    for( i = 0 ;i < height;i++){
      memcpy(ptr,frame->data[0] + i * frame->linesize[0],width);
      ptr += width;
    }
    //copy data u
    for(i=0;i < height/2;i++){
      memcpy(ptr,frame->data[1] + i * frame->linesize[1],width/2);
      ptr += width/2;
    }
    //copy data v
    for(i=0;i < height/2;i++){
      memcpy(vedio_data,frame->data[2] + i * frame->linesize[2],width/2);
      ptr += width/2;
    }
    //
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    int pitch = width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_YV12);
    SDL_Texture * text = SDL_CreateTexture(ctx->render,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,width,height);
    
    SDL_UpdateTexture(text,&rect,vedio_data,pitch);
    SDL_RenderClear(ctx->render);
    SDL_RenderCopy(ctx->render,text,NULL,NULL);
    SDL_RenderPresent(ctx->render);
    return 0;
  }
  return 0;
}


static int  input_data(struct player_context * ctx,char * buf,int len){
  if(decode_rtp2h264(ctx,buf,len) == CONTINUE){
    return 0 ;//do nothing ,wait next packet
  }else{
    return do_play(ctx);
  }
}

static int_fast32_t
linit_player(lua_State * L){
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)){
    return luaL_error(L,"can not start sdl video");
  }
  avcodec_register_all();
  return 0;
}


static int
lnew_player(lua_State * L){
  int win_width,win_height;
  HWND handle;
  AVCodecContext * c;
  win_width = luaL_checkinteger(L,1);
  win_height = luaL_checkinteger(L,2);
  handle = luaL_checkinteger(L,3);
  AVCodec * codec = avcodec_find_decoder(CODEC_ID_H264);
  if(!codec){
    return luaL_error(L,"can not find h264 codec");
  }
  c = avcodec_alloc_context3(codec);
  if(!c){
    return luaL_error(L,"can not alloc codec context");
  }
  if (avcodec_open2(c, codec,NULL) < 0) {
    return luaL_error(L,"h264 codec didn't support");
  }
  
  struct player_context * ctx = malloc(sizeof(*ctx));
  ctx->buf_pos = ctx->buf;
  ctx->buf_len = 0;
  if(!ctx){
    return luaL_error(L,"malloc play context error");
  }
  
  SDL_Window * win = SDL_CreateWindowFrom(handle);
  if(!win){
    return luaL_error(L,"create window error");
  }
  int num = SDL_GetNumRenderDrivers();
  SDL_Renderer * r = SDL_CreateRenderer(win,0,SDL_RENDERER_ACCELERATED);
  if(!r){
    return luaL_error(L,"create render error");
  }
  ctx->av_ctx = c ; 
  ctx->window = win;
  ctx->render = r;
  ctx->win_width = win_width;
  ctx->win_height = win_height;
  lua_pushlightuserdata(L,ctx);
  return 1;
}

static int
ldestory_player(lua_State *L){
  struct plyer_context * ctx  = lua_touserdata(L,1);
  //free(ctx->win);
  //free(ctx->render);
  free(ctx);
  return 0;
}
static int
lsend(lua_State * L){
  struct player_context * ctx  = lua_touserdata(L,1);
  char * buf = lua_touserdata(L,2);
  int len = luaL_checkinteger(L,3);
  input_data(ctx,buf,len);
  return 0;
}

int
luaopen_hiveplayer_c(lua_State *L) {
  luaL_checkversion(L);
  luaL_Reg l[] = {
    {"init_player",linit_player},
    {"new_player",lnew_player},
    {"send",lsend},
    {"destroy_player",ldestory_player},
    {NULL,NULL},
  };
  luaL_newlib(L,l);
  return 1;
}





