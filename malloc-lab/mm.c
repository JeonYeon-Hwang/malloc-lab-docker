/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
// #define DEBUG

#ifdef DEBUG
    #define dbg_printf(...) printf(__VA_ARGS__)
#else
    #define dbg_printf(...)
#endif

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define GET_SIZE(p) (*(size_t *)(p) & ~0x7) // 해당 주소의 8바이트 값 읽어서 size 도출
#define GET_ALLOC(p) (*(size_t *)(p) & 0x1) // 해당 주소의 1바이트 값 읽어서 할당유무 확인
#define HDRP(bp) ((char *)bp - SIZE_T_SIZE) // payload 주소값으로 header 찾기
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - (2 * SIZE_T_SIZE)) // payload 주소값으로 footer 찾기

// 얘들은 일단 복붙
#define WSIZE (SIZE_T_SIZE)
#define DSIZE (2 * SIZE_T_SIZE)
#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define NEXT_BLKP(bp) (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))) 

static char *heap_listp; // heap의 시작 주소 => 1바이트 단위 계산을 위해 char로 설정

static void *find_fit(size_t put_size); // implicit 구현: 사이 탐색 함수
static void *coalesce(void *bp); // 인접 free 공간 병합 함수
static void place(void *bp, size_t new_size); // 기존 free 공간에 할당/분할 함수


/*
 * mm_init - initialize the malloc package.
 */
// 메모리를 초기화 하는 로직: 한 테스트 시나리오 종료 시 발동
int mm_init(void)
{
    char *p = mem_sbrk(4 * SIZE_T_SIZE); // 32바이트 확보
    if (p == (void *)-1) return -1;
    
    *(size_t *)p = 0; // 패딩: 모두 0으로 채우기
    *(size_t *)(p + SIZE_T_SIZE) = ((2 * SIZE_T_SIZE) | 1); // 프롤로그 헤더 => size가 8, 할당상태
    *(size_t *)(p + (2 * SIZE_T_SIZE)) = ((2 * SIZE_T_SIZE) | 1); // 프롤로그 푸터 => size: 8, 할당상태
    *(size_t *)(p + (3 * SIZE_T_SIZE)) = (0 | 1); // 에필로그 헤더 => size가 0, 할당상태

    heap_listp = p + (2 * SIZE_T_SIZE); // 힙 시작 주소 설정: 사실상 payload 주소(여기선 푸터의 시작점)
    
    
    dbg_printf("\nmm_init:  heap_strt: %p (0 bytes) \n\n", heap_listp);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// 메모리를 할당하는 로직: 현재 기본적인 것만 구현되어 있음
// 여기서 이해한 내용: malloc은 실제 모두 채워넣는 것이 아닌, header(명표)만 만들어 놓는다.
void *mm_malloc(size_t size) // void와 void *는 다름 .. size_t => 메모리 크기를 표현하는 타입
{
    int newsize = ALIGN(size + (2 * SIZE_T_SIZE));  // header를 위한 8바이트 추가
    dbg_printf("mm_malloc 실행 => %8d %8d", newsize, (int)size);
    
    void *internal_p = find_fit(newsize);
    if(internal_p != (void *)-1){
        place(internal_p, newsize);
              
        dbg_printf("  =>  내부 찾음   header: %p (%ld bytes)\n", HDRP(internal_p), (char *)HDRP(internal_p) - (char *)heap_listp); 
        return(void *)internal_p; // 사용자가 할당할 payload 주소를 반환
    }
    
    void *p = mem_sbrk(newsize);  // p는 mem_sbrk로 확장된 heap의 payload 주소값
    if (p == (void *)-1){
        dbg_printf("=> 공간이 부족합니다(실패)!  \n");  // 에러 결과값(-1)에 따른 방어 코드
        return NULL;
    }   
    else
    {
        *(size_t *)HDRP(p) = (newsize | 1); // (구) 에필로그 자리에 header를 박는 메서드
        *(size_t *)FTRP(p) = (newsize | 1); // 확장된 heap의 새 footer
        *(size_t *)(FTRP(p) + SIZE_T_SIZE) = (0 | 1); // 확장된 heap의 새 에필로그

        dbg_printf("  =>  힙 팽창함   header: %p (%ld bytes)\n", HDRP(p), HDRP(p) - (char *)heap_listp);
        
        return (void *)((char *)p); // 다시 (char *)로 형으로 
                                    // payload 값으로 반환                                           
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
// 메모리를 해제하는 로직
// header와 footer 모두 size_t 형식의 데이터 양을 명시한다
void mm_free(void *ptr) // ptr 주소 = payload 주소
{
    if(ptr == NULL) return;

    size_t size = GET_SIZE(HDRP(ptr)); // payload 기반 size 추출
    PUT(HDRP(ptr), PACK(size, 0)); //payload 기반 헤더 할당해제
    PUT(FTRP(ptr), PACK(size, 0)); //payload 기반 푸터 할당해제
    
    dbg_printf("매모리 해제    =>  header: %8p   size: %d bytes\n", HDRP(ptr), (int)(size));
    
    coalesce(ptr); // 병합하기
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// 메모리 재할당 로직: 축소 and 확장 모두 가능하도록 지원해야 함.
// 동적 메모리 할당 => 국소적 관리를 이용할 것
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;

    size_t copySize = GET_SIZE(HDRP(ptr)) - 2 * SIZE_T_SIZE;
    if (size < copySize) copySize = size;

    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}




// implicit 구현: 시작부터 ==> 에필로그 헤더까지 순회
static void *find_fit(size_t put_size){
    void *bp; // current 포인터 역할

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = (char *)bp + GET_SIZE(HDRP(bp))){ // 반복문:  
        size_t header_value = *(size_t *)HDRP(bp); // 해당 bp의 헤더의 값을 추출
        int is_allocated = header_value & 0x1; // 해당 헤더가 할당되어 있는지 알아보기
        if(!is_allocated && (put_size <= GET_SIZE(HDRP(bp)))){ // 할당 가능 && 사이즈 더 작을 시 포인터 반환
            return bp; // payload로 header로 접근하려면: -8 
        }
    }

    return (void *)-1; // 없을 경우 -1 반환
}




// 병합 함수: mm_free & extend_heap 이벤트에서 호출 => 4가지 경우의 수
static void *coalesce(void *bp){ 
    char *prev_bp = (char *)bp - GET_SIZE((char *)(bp) - (2 * SIZE_T_SIZE)); // 이전 블록의 payload
    char *next_bp = (char *)bp + GET_SIZE((char *)(bp) - (SIZE_T_SIZE)); // 다음 블록의 payload

    size_t prev_alloc = GET_ALLOC(FTRP(prev_bp)); // 현 payload 기준 => 푸터로 이동하여 할당유무 확인
    size_t next_alloc = GET_ALLOC(HDRP(next_bp)); // 현 payload 기준 => 헤더로 이동하여 할당유무 확인
    size_t size = GET_SIZE(HDRP(bp)); // 현 payload 기준 => 블록의 size 추출

    if(prev_alloc && next_alloc){ // 앞 뒤 블록이 모두 할당되어 있다면?
        return bp; // 변화 없음
    }
    else if(prev_alloc && !next_alloc){ // 뒤 블록만 free라면?
        size += GET_SIZE(HDRP(next_bp)); // 뒤 블록의 size를 더함
        *(size_t *)HDRP(bp) = (size | 0); // 현 블록 헤더 size 확장 
        *(size_t *)FTRP(bp) = (size | 0); // 현 블록 푸터 size 확장 
    }
    else if(!prev_alloc && next_alloc){ // 앞 블록만 free라면?
        size += GET_SIZE(HDRP(prev_bp)); // 앞 블록의 size를 더함
        *(size_t *)HDRP(prev_bp) = (size | 0); // 앞 블록 헤더 size 확장
        *(size_t *)FTRP(prev_bp) = (size | 0); // 현 블록 푸터 size 확장
        bp = prev_bp; // bp를 앞 블록 payload로 이동
    }
    else if(!prev_alloc && !next_alloc){ // 앞 뒤 블록 모두 free라면?
        size += (GET_SIZE(HDRP(next_bp)) + GET_SIZE(HDRP(prev_bp))); // 앞 뒤 블록의 size를 모두 더함
        *(size_t *)HDRP(prev_bp) = (size | 0); // 앞 블록의 헤더 size 확장
        *(size_t *)FTRP(prev_bp) = (size | 0); // 뒤 블록의 푸터 size 확장
        bp = prev_bp; // bp를 앞 블록 payload로 이동
    }
    dbg_printf("병합됨         =>  header: %p(%ld bytes)   size:%d bytes\n", HDRP(bp), HDRP(bp) - (char *)heap_listp, (int)(size));
    return bp;
}



// 배치 함수
static void place(void *bp, size_t new_size){ // 받는 인자: 할당 payload & 할당할 size
    size_t free_size = GET_SIZE(HDRP(bp)); // 먼저 free_size를 구한다
    int free_amount = free_size - new_size; // 여분이 될 free_size

    if(free_amount >= DSIZE){ // 여분의 공간이 16바이트 이상이라면?
        PUT(HDRP(bp), PACK(new_size, 1)); // 헤더 new_size로 갱신
        PUT(FTRP(bp), PACK(new_size, 1)); // 푸터 new_size로 갱신

        void *next_bp = (char *)bp + new_size; // 여분이 될 free_size의 payload

        PUT(HDRP(next_bp), PACK(free_amount, 0)); // 여분 헤더 생성
        PUT(FTRP(next_bp), PACK(free_amount, 0)); // 여분 푸터 생성
    }
    else{ // 여분의 공간이 16 바이트 미만이라면? => 내부 단편화 감수
        PUT(HDRP(bp), PACK(free_size, 1));
        PUT(FTRP(bp), PACK(free_size, 1));
    }
}

// short1-bal.rep: Perf index = 30 (util) + 7 (thru) = 37/100 
// short1-bal.rep: Perf index = 40 (util) + 40 (thru) = 80/100
// 초창기 통합 테스트: Perf index = 46 (util) + 16 (thru) = 62/100