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

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


static char *heap_listp; // heap의 시작 주소 => 1바이트 단위 계산을 위해 char로 설정

static void *find_fit(size_t put_size); // implicit 구현: 사이 탐색 함수



/*
 * mm_init - initialize the malloc package.
 */
// 메모리를 초기화 하는 로직: 한 테스트 시나리오 종료 시 발동
int mm_init(void)
{
    char *p = mem_sbrk(3 * SIZE_T_SIZE); // 24바이트 확보
    if (p == (void *)-1) return -1;
    
    *(size_t *)p = 0; // 패딩: 모두 0으로 채우기
    *(size_t *)(p + SIZE_T_SIZE) = (SIZE_T_SIZE | 1); // 프롤로그 헤더 => size가 8, 할당상태
    *(size_t *)(p + (2 * SIZE_T_SIZE)) = (0 | 1); // 에필로그 헤더 => size가 0, 할당상태

    heap_listp = p + (2 * SIZE_T_SIZE); // 힙 시작 주소 설정: 사실상 payload 주소
    
    
    printf("\nmm_init: 모든 메모리가 초기화 됩니다. 힙 시작주소: %p \n\n", heap_listp);
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
    int newsize = ALIGN(size + SIZE_T_SIZE);  // header를 위한 8바이트 추가
    printf("mm_malloc: 점유 공간 | 실제 메모리 양: %8d | %8d  ", newsize, (int)size);
    
    void *p = mem_sbrk(newsize) - SIZE_T_SIZE;  // p라는 변수에 "주소 숫자"를 저장함.
                                                // payload 임으로 - 8 하여 header 생성 위치로 이동.
                                                // 요 함수는 단순 영토 확장(에필로그 헤더를 넓히는) 역할을 함.
    if (p == (void *)-1){
        printf("=> 공간이 부족합니다(실패)! \n");  // 에러 결과값(-1)에 따른 방어 코드
        return NULL;
    }   
    else
    {
        *(size_t *)p = size; // (구) 에필로그 자리에 header를 박는 메서드
                             // p가 가리키는 메모리 그 자체에 "블록 크기(size)"를 저장함. 
                             // (선언* 와 이후*는 다름)
                             // 만약 (size_t)p => 강제로 양수로 형변환 됨 
                             // (size_t *) => 해당 p(주소값)은 size_t를 기록한 메모리의 주소값이라고 가정
                             // * => 그리고 그 주소값으로 가
                             // 따라서: p가 가리키는 실제 주소값에 size라는 양의 정수가 header(8바이트)에 입력됨
        *(size_t *)((char *)p + newsize) = (0 | 1); // 에필로그를 뒤로 밀어서 생성
        printf(" ===>   공간을 할당하였습니다! .. 명표(header) 위치: %p \n", p);
        return (void *)((char *)p + SIZE_T_SIZE); // 다시 (char *)로 형 변환 
                                                  // size_t(8바이트) => char(1바이트) 로 형 변환
                                                  // 8 + 1 = 9번째 주소값을 반환: 데이터 입력이 시작되는 주소값
                                                  // void * => 타입명 다시 변경(무엇이든 다시 가능)                                               
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
// 메모리를 해제하는 로직
// header와 footer 모두 size_t 형식의 데이터 양을 명시한다
void mm_free(void *ptr) // ptr 주소 = payload 주소(header + 8)
{
    void *header = ((char *)ptr - SIZE_T_SIZE); // payload => header추출
    size_t size = *(size_t *)header; // header => size_t(정수화)
    *(size_t *)header = size & ~0x1; // 마지막 비드를 0으로 만들어 해제
    
    printf("mm_free: 다음 공간이 해제되었습니다:      header|   size_t   ===>   %8p | %8d\n", header, (int)(size + SIZE_T_SIZE));
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// 메모리 재할당 로직: 축소 and 확장 모두 가능하도록 지원해야 함.
// 동적 메모리 할당 => 국소적 관리를 이용할 것
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    printf("mm_realloc: 공간 재할당 시도 ");
    if (newptr == NULL){
        printf("=> 재할당 공간이 부족합니다(실패)! \n");
        return NULL;
    }
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    printf("=> newptr <-> oldptr 간 재조정이 일어났습니다! \n");
    mm_free(oldptr);
    return newptr;
}




// implicit 구현: 시작부터 ==> max_heap 까지 순회
static void *find_fit(size_t put_size){
    void *last_byte = mem_heap_hi(); // 현재까지 사용하였던 최대 header 위치
    void *bp = heap_listp; // current 포인터 역할 => heap 시작 위치로 초기화

    while(1){

    }

    return (void *)-1; // 없을 경우 -1 반환
}


// short1-bal.rep 기본점수: Perf index = 30 (util) + 7 (thru) = 37/100 .. 37점