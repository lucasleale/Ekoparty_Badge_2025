#ifndef AMY_PCM_SET
#define AMY_PCM_SET 0   // 0 = TINY, 1 = CUSTOM1, 2 = CUSTOM2
#endif

#if AMY_PCM_SET == 1
  #include "pcm_custom1.h"
#elif AMY_PCM_SET == 2
  #include "pcm_custom2.h"
#else
  //#include "pcm_tiny.h" //EKOPARTYYYY
  #include "pcm_small.h"
#endif