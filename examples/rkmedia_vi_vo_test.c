// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common/sample_common.h"
#include "rkmedia_api.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

int main(int argc, char *argv[]) {
  int ret = 0;
  int video_width = 1920;
  int video_height = 1080;
  int video1_width = 640;
  int video1_height = 360;
  int disp_widht = 720;
  int disp_height = 1280;
  int disp1_width = 360;
  int disp1_height = 640;

  RK_MPI_SYS_Init();
#ifdef RKAIQ
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  RK_BOOL fec_enable = RK_FALSE;
  int fps = 30;
  char *iq_file_dir = NULL;
  if ((argc > 1) && !strcmp(argv[1], "-h")) {
    printf("\n\n/Usage:./%s [--aiq iq_file_dir]\n", argv[0]);
    printf("\t --aiq iq_file_dir : init isp\n");
    return -1;
  }
  if (argc == 3) {
    if (strcmp(argv[1], "--aiq") == 0) {
      iq_file_dir = argv[2];
    }
  }
  SAMPLE_COMM_ISP_Init(hdr_mode, fec_enable, iq_file_dir);
  SAMPLE_COMM_ISP_Run();
  SAMPLE_COMM_ISP_SetFrameRate(fps);
#else
  (void)argc;
  (void)argv;
#endif
  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 4;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = "rkispp_scale1";
  vi_chn_attr.u32BufCnt = 4;
  vi_chn_attr.u32Width = video1_width;
  vi_chn_attr.u32Height = video1_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, 1);
  if (ret) {
    printf("Create vi[1] failed! ret=%d\n", ret);
    return -1;
  }

  // rga0 for primary plane
  RGA_ATTR_S stRgaAttr;
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 12;
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_RGB888;
  stRgaAttr.stImgOut.u32Width = disp_widht;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_widht;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }

  // rga1 for overlay plane
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 12;
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video1_width;
  stRgaAttr.stImgIn.u32Height =  video1_height;
  stRgaAttr.stImgIn.u32HorStride = video1_width;
  stRgaAttr.stImgIn.u32VirStride =   video1_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgOut.u32Width = disp1_width;
  stRgaAttr.stImgOut.u32Height = disp1_height;
  stRgaAttr.stImgOut.u32HorStride = disp1_width;
  stRgaAttr.stImgOut.u32VirStride = disp1_height;
  ret = RK_MPI_RGA_CreateChn(1, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }

  VO_CHN_ATTR_S stVoAttr = {0};
  // VO[0] for primary plane
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
  stVoAttr.enImgType = IMAGE_TYPE_RGB888;
  stVoAttr.u16Fps = 60;
  stVoAttr.u16Zpos = 0;
  stVoAttr.u32Width = disp_widht;
  stVoAttr.u32Height = disp_height;
  stVoAttr.u32HorStride = disp_widht;
  stVoAttr.u32VerStride = disp_height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  // VO[1] for overlay plane
  memset(&stVoAttr, 0, sizeof(stVoAttr));
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.enImgType = IMAGE_TYPE_NV12;
  stVoAttr.u16Fps = 60;
  stVoAttr.u16Zpos = 1;
  stVoAttr.u32Width = disp1_width;
  stVoAttr.u32Height = disp1_height;
  stVoAttr.u32HorStride = disp1_width;
  stVoAttr.u32VerStride = disp1_height;
  ret = RK_MPI_VO_CreateChn(1, &stVoAttr);
  if (ret) {
    printf("Create vo[1] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  printf("#Bind VI[0] to RGA[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("# Bind RGA[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("#Bind VI[1] to RGA[1]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }

  printf("# Bind RGA[1] to VO[1]....\n");
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind rga[1] to vo[1] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(100);
  }

  printf("%s exit!\n", __func__);
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[1] to vo[1] failed! ret=%d\n", ret);
    return -1;
  }

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(); // isp aiq stop before vi streamoff
#endif
  RK_MPI_VO_DestroyChn(0);
  RK_MPI_VO_DestroyChn(1);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_RGA_DestroyChn(1);
  RK_MPI_VI_DisableChn(0, 0);
  RK_MPI_VI_DisableChn(0, 1);

  return 0;
}
