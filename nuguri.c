// 크로스 플랫폼 호환을 위한 공통 헤더
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Windows 환경 감지
#ifdef _WIN32
    #include <windows.h> // Sleep(), system("cls") 등 사용
    #include <conio.h> // getch(), kbhit() 등 사용
// Linux 또는 macOS 환경
#else // WIN32 매크로가 정의되지 않음 -> Linux/Mac 환경일때 정의됨
    #include <unistd.h> // usleep(), read() 등 사용
    #include <termios.h> // 터미널 속성 제어(버퍼링/에코 비활성화 등)
    #include <fcntl.h> // 논블로킹 입력 등
#endif // 운영체제 분기 종료

// 맵 및 게임 요소 정의 (수정된 부분)
#define MAX_ENEMIES 15 // 최대 적 개수 증가
#define MAX_COINS 30   // 최대 코인 개수 증가

// 구조체 정의
// 적 정보
typedef struct {
    int x, y; // 적의 현재 좌표 (맵 상의 x, y 위치)
    int dir; // 1: right, -1: left
} Enemy;

// 코인 정보
typedef struct {
    int x, y; // 코인 위치
    int collected; // 0: 아직 먹지 않음, 1: 먹음
} Coin;

// 사운드 정의
typedef enum {
    sound_COIN, // 코인 먹을때
    sound_ENEMY, // 적과 충돌했을때
    sound_CLEAR, // 스테이지 클리어
    sound_GAMEOVER // 게임 오버
} Play;

// 전역 변수
char ***map; // 3차원 배열 포인터 (stage, y, x)
// 맵,스테이지 크기 전역 변수 선언
int map_width = 0; // 맵 가로 길이
int map_height = 0; // 맵 세로 길이
int MAX_STAGES = 0; // 전체 스테이지 개수
// 맵 제한 크기
int limit_width = 256;
int limit_height = 256;
// 플레이어 상태 관련 전역 변수
int player_x, player_y; // 플레이어 현재 좌표
int stage = 0; // 현재 스테이지
int score = 0; // 현재 점수
int life = 3; // 남은 목숨 수

// 플레이어 점프/사다리 상태
int is_jumping = 0; // 점프및 낙하 상태
int velocity_y = 0; // 수직 속도
int on_ladder = 0; // 사다리 여부 확인

// 게임 객체
Enemy enemies[MAX_ENEMIES]; // 적 배열
int enemy_count = 0; // 현재 스테이지의 적 개수
Coin coins[MAX_COINS]; // 코인 배열
int coin_count = 0; // 코인 개수

// Linux와 macOS 환경에서 사용할 터미널 설정
#ifndef _WIN32
    // 터미널 설정
    struct termios orig_termios;
#endif

// 함수 선언
// 터미널 모드 관련
void disable_raw_mode();
void enable_raw_mode();
// 맵 및 스테이지 처리
void load_maps();
void init_stage();
void free_maps();
// 게임 루프와 화면 처리
void draw_game();
void update_game(char input);
// 이동 및 충돌 처리
void move_player(char input);
void move_enemies();
void check_collisions();
// 플랫폼별 함수
int kbhit();
void clrscr();
void delay(int ms);
int getch(void);
void gotoxy(int x, int y);
void hide_cursor();
void show_cursor();
// 게임 흐름 화면
void title();
void game_over();
void ending();
// 사운드 함수
void playsound(Play type);

int main() {
    // Windows 콘솔을 UTF-8 모드로 설정 : 한글 깨짐 방지
    #ifdef _WIN32
        SetConsoleOutputCP(65001); // UTF-8 출력
        SetConsoleCP(65001); // UTF-8 입력
    #endif
    srand(time(NULL)); // 랜덤 시드 설정 (적 방향 랜덤 초기화 등에 사용)
    load_maps(); // map.txt를 읽어서 맵과 스테이지 정보 동적 할당
    title(); // 타이틀 화면
    init_stage(); // 현재 스테이지 기준 플레이어, 적, 코인 위치 초기화

    char c = '\0'; // 최근에 입력된 키 값 저장
    int game_over = 0; // 게임 종료 여부

    // 메인 게임 루프
    while (!game_over && stage < MAX_STAGES) {
        // 키보드 입력이 있는지 확인
        if (kbhit()) {
            while(kbhit()) { // 버퍼 비우기
	            c = getch(); // 한글자 읽기

	#ifdef _WIN32
	    // Windows 방향키 입력 처리
	    if (c == 0 || c == -32 || c == 224) { //224 : unsigned, -32 : signed char
	        int arrow = getch(); // 실제 방향 값 읽기
	        switch (arrow) {
	            case 72: c = 'w'; break; // 위
	            case 80: c = 's'; break; // 아래
	            case 75: c = 'a'; break; // 왼쪽
	            case 77: c = 'd'; break; // 오른쪽
	        }
	    }
	#else
	    // Linux 방향키 처리
	    if (c == '\033') {
	        getch(); // '['
	        switch (getch()) { // 실제 방향 코드
	            case 'A': c = 'w'; break; // 위
	            case 'B': c = 's'; break; // 아래
	            case 'C': c = 'd'; break; // 오른쪽
	            case 'D': c = 'a'; break; // 왼쪽
	        }
	    }
	#endif
            // q 입력 시 게임 종료
	        if (c == 'q') {
	            game_over = 1;
	            continue; // 다음 루프
	        }
        }
	}
	
	else {
            c = '\0'; // 입력 없음
        }

        update_game(c); // 입력에 따라 플레이어 이동/적이동/충돌 등 게임 상태 갱신
        draw_game(); // 현재 상태를 화면에 다시 그리기
        delay(90); // 속도 조절

        // 'E' 즉 출구인 경우 스테이지 클리어
        if (map[stage][player_y][player_x] == 'E') {
            stage++; // 다음 스테이지 이동
            score += 100; // 클리어 보너스 점수
            playsound(sound_CLEAR); // 클리어 사운드
            if (stage < MAX_STAGES) { 
                init_stage(); // 남은 스테이지가 있는 경우 다음 스테이지 초기화
            } else {
                // 모든 스테이지 클리어
                game_over = 1;
                ending(); // 엔딩 화면 출력
            }
        }
    }

    // 메인 루프 종료
    free_maps(); // 동적 할당된 맵 해제
    disable_raw_mode(); // 터미널 모드 복원
    show_cursor(); // 숨긴 커서 다시 표시
    return 0;
}

// 맵 파일 로드
void load_maps() {
    FILE *file = fopen("map.txt", "r"); // 읽기 전용으로 map.txt 오픈
    if (!file) {
        // 파일 열기 실패시 에러 메세지 표시 후 프로그램 종료
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }
    // 맵 크기, 스테이지 수 읽기
    char buf[256]; // 한 줄을 읽을 버퍼
    int max_width = 0; // 최대 너비
    int max_height = 0; // 최대 높이
    int current_height = 0; // 현재 스테이지 높이
    int stage_count = 1; // 최소 1 스테이지 존재

    while(fgets(buf, sizeof(buf), file)) {
        buf[strcspn(buf, "\r\n")] = 0; // 개행 문자 제거

        int len = strlen(buf); // 현재 줄 길이(length)

        if(len == 0) { // 빈 줄(스테이지 구분)
            if(current_height > 0) { // 현재 스테이지 높이 갱신
                stage_count++; // 빈 줄 만났을때 스테이지 수 증가
                if(current_height > max_height) { // 최대 높이 갱신
                    max_height = current_height;
                }
                current_height = 0; // 현재 높이 초기화
            }
            continue;
        }

        if(len > max_width) { // 최대 너비 갱신
            max_width = len;
        }
        current_height++; // 현재 스테이지 높이 증가
    }
    if(current_height > 0 && current_height > max_height) { // 최대 높이 갱신
        max_height = current_height;
    }

    // 맵 크기가 제한을 넘어가면 경고 출력
    if(max_width > limit_width) {
        max_width = limit_width; // 제한 너비 적용
        printf("경고: 맵 너비가 제한을 초과하여 %d로 조정됩니다.\n", limit_width);
        getch();
    }
    if(max_height > limit_height) {
        max_height = limit_height; // 제한 높이 적용
        printf("경고: 맵 높이가 제한을 초과하여 %d로 조정됩니다.\n", limit_height);
        getch();
    }

    // 계산된 값 전역변수에 반영
    MAX_STAGES = stage_count; // 스테이지 수 갱신
    map_width = max_width;
    map_height = max_height;

    // 맵 동적 할당
    map = (char ***)malloc(sizeof(char **) * MAX_STAGES); // 스테이지 포인터 동적 할당
    int i = 0;
    for(i = 0; i < MAX_STAGES; i++) {
        map[i] = (char **)malloc(sizeof(char *) * max_height); // y 포인터 동적 할당
        for(int j = 0; j < max_height; j++) {
            map[i][j] = (char *)malloc(sizeof(char) * max_width + 1); // x 포인터 동적 할당, NULL 문자용 +1
            memset(map[i][j], ' ', max_width); // 공백으로 초기화
            map[i][j][max_width] = '\0'; // 문자열 종료
        }
    }

    rewind(file); // 파일 포인터 처음으로 되돌리기

    int s = 0, y = 0; // s: 스테이지 인덱스, y: 현재 스테이지 높이
    while(fgets(buf, sizeof(buf), file) && s < MAX_STAGES) { // 스테이지 수만큼 읽기
        buf[strcspn(buf, "\r\n")] = 0; // 개행 문자 제거
        int len = strlen(buf);

        if(len == 0) { // 빈 줄(스테이지 구분)
            if(y > 0) {
                s++; // 스테이지 증가
                y = 0; // y 초기화
            }
            continue;
        }

        if(y < max_height) {
            memcpy(map[s][y], buf, len); // 읽어온 줄 맵 메모리에 복사
            y++; // 읽어온 길이만큼만 복사, 나머지는 공백 유지
        }
    }
    fclose(file); // 파일 닫기
}       


// 현재 스테이지 초기화
void init_stage() {
    // 각종 상태 초기화
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    // 현재 스테이지 전체를 돌며 오브젝트 위치 탐색
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            char cell = map[stage][y][x]; // 현재 타일 문자
            if (cell == 'S') {
                // 시작 위치 'S' -> 플레이어 좌표 설정
                player_x = x;
                player_y = y;
            } else if (cell == 'X' && enemy_count < MAX_ENEMIES) {
                // 적 'X' -> enemies 배열에 추가
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1}; // 왼/오 방향 랜덤
                enemy_count++;
            } else if (cell == 'C' && coin_count < MAX_COINS) {
                // 코인 'C' -> coins 배열에 추가
                coins[coin_count++] = (Coin){x, y, 0};
            }
        }
    }
}

// 게임 화면 그리기
void draw_game() {
    #ifdef _WIN32
        gotoxy(1, 1); // 윈도우에서는 좌표만 이동
    #else
        clrscr(); // 유닉스 계열에서는 화면 전체 클리어
    #endif
    // 스테이지, 점수, 라이프, 조작 키 소개
    printf("Stage: %d | Score: %d\n", stage + 1, score);
    printf("Life :%d ", life);
    for(int i=0; i<life; i++) printf("❤"); // 남은 생명만큼 하트 출력
    printf("\n조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");

    // 표시용 맵 버퍼
    char display_map[map_height][map_width + 1];

    for(int y=0; y < map_height; y++) {
        for(int x=0; x < map_width; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C') {
                // 시작점, 적, 코인은 공백 -> 나중에 추가
                display_map[y][x] = ' ';
            } else {
                // 그 외는 그대로 복사
                display_map[y][x] = cell;
            }
        }
    }
    
    // 아직 먹지 않은 코인만 표시
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected) {
            display_map[coins[i].y][coins[i].x] = 'C';
        }
    }

    // 적 위치 표시
    for (int i = 0; i < enemy_count; i++) {
        display_map[enemies[i].y][enemies[i].x] = 'X';
    }

    // 플레이어 표시
    display_map[player_y][player_x] = 'P';

    // 완성된 맵 전체를 콘솔에 출력
    for (int y = 0; y < map_height; y++) {
        for(int x=0; x< map_width; x++){
            printf("%c", display_map[y][x]);
        }
        printf("\n");
    }
}

// 게임 상태 업데이트
void update_game(char input) {
    move_player(input); // 플레이어 이동 처리
    move_enemies(); // 적 이동 처리
    check_collisions(); // 충돌 체크
}

// 플레이어 이동 로직
void move_player(char input) {
    int before_x = player_x; // 이동 전 x위치 저장
    int next_x = player_x, next_y = player_y; // 이동 좌표

    // 발밑 타일
    char floor_tile = (player_y + 1 < map_height) ? map[stage][player_y + 1][player_x] : '#';
    // 현재 타일
    char current_tile = map[stage][player_y][player_x];
    // 현재 위치가 사다리인지 여부
    on_ladder = (current_tile == 'H');

    // 이동
    switch (input) {
        case 'a': next_x--; break; //왼쪽 이동
        case 'd': next_x++; break; // 오른쪽 이동
        case 'w': if (on_ladder) next_y--; break; // 위로 이동
        case 's': if (on_ladder && (player_y + 1 < map_height) && map[stage][player_y + 1][player_x] != '#') next_y++; break; // 아래로 이동
        case ' ':
            // 점프
            if (!is_jumping && (floor_tile == '#' || on_ladder)) {
                is_jumping = 1; // 점프 상태 진입
                velocity_y = -2; // 위로 향하는 초기 속도(-2)
            }
            break;
    }

    // 가로 이동 -> 벽이 아닐때만 이동
    if (next_x >= 0 && next_x < map_width && map[stage][player_y][next_x] != '#') player_x = next_x;
    // 사다리 아래쪽으로 내려감
    if(input == 's' && player_y +2 < map_height && map[stage][player_y + 2][player_x] == 'H' && map[stage][player_y + 1][player_x] != 'H') { // 사다리 내려가기 구현
        next_y = player_y + 2; // 바닥 밑 사다리가 존재하면 2칸 아래로 이동
        player_y = next_y;
        is_jumping = 0;
        velocity_y = 0;
        return;
    }
    
    // 사다리 위, 아래 이동
    if (on_ladder && (input == 'w' || input == 's')) {
        // 사다리 이동 위치가 맵 범위 안이고 벽이 아니라면 이동
        if(next_y >= 0 && next_y < map_height && map[stage][next_y][player_x] != '#') {
            player_y = next_y;
            is_jumping = 0;
            velocity_y = 0;
        } else if (input == 'w' && next_y >= 0 && map[stage][next_y][player_x] == '#') { // 위로 올라갈때 다음칸이 '#'일 경우
            // 벽 위칸이 비어있는지 확인
            if(next_y-1 >= 0 && map[stage][next_y-1][player_x] != '#') {
                player_y = next_y - 1; // 플레이어 위치를 벽 위로 이동
                is_jumping = 0; // 점프 상태 해제
                velocity_y = 0; // 속도 상태 해제
            }
        }
    } 
    else {
        // 점프 중일때
        if (is_jumping) {
            int step = (velocity_y < 0) ? -velocity_y : velocity_y; // 현재 이동한 속도 절댓값 계산
            int mov = (velocity_y < 0) ? -1 : 1; // 이동방향 : 음수 -> 위(-1), 양수 -> 아래(1)

            // 속도만큼 1칸씩 이동하며 충돌 체크
            for(int i = 0; i < step; i++) {
                int ch_y = player_y + mov; // 1칸 이동했을때 위치 확인

                // 천장, 바닥 충돌 체크
                if(ch_y < 0 || ch_y >= map_height) {
                    velocity_y = 0; // 속도 멈춤
                    if(ch_y < 0) ch_y = 0; // 천장 뚫고 나가는것 금지
                    break;
                }

                // 벽 충돌 체크
                if(map[stage][ch_y][player_x] == '#') {
                    velocity_y = 0; // 충돌시 속도 0
                    if(mov == 1) is_jumping = 0; // 아래 충돌시 착지
                    break;
                }
                player_y = ch_y; // 충돌 없으면 1칸 이동

                check_collisions(); // 이동 후 충돌 체크
            }

            // 중력 적용
            if(is_jumping) {
                velocity_y++; // 중력 가속도 : 속도 증가
                if(velocity_y > 2) velocity_y = 2; // 낙하속도 2로 제한
            }
        } else {
            // 점프 중이 아닐때 떨어짐 감지
            if (floor_tile != '#' && floor_tile != 'H') {
                is_jumping = 1; // 점프 상태
                velocity_y = 1; // 낙하 속도 적용
            }
        }
    }
    
    // 맵 아래로 떨어진 경우 스테이지를 다시 초기화
    if (player_y >= map_height) init_stage();

    // 벽 끼임 확인 -> x,y 되돌리기
    if (player_x >= 0 && player_x < map_width && 
        player_y >= 0 && player_y < map_height &&
        map[stage][player_y][player_x] == '#') {
        player_x = before_x; // 벽인면 x좌표를 이전값으로 되돌림
    }
}

// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        // 맵 범위 벗어남, 이동위치가 벽, 아래가 공중인 경우 방향 전환
        if (next_x < 0 || next_x >= map_width || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < map_height && map[stage][enemies[i].y + 1][next_x] == ' ')) {
            enemies[i].dir *= -1; // 이동 방향 반전
        } else {
            enemies[i].x = next_x; // 좌우로 한칸 이동
        }
    }
}

// 충돌 감지 로직
void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
        if (player_x == enemies[i].x && player_y == enemies[i].y) {
            life--; // 목숨 1 감소
            playsound(sound_ENEMY); // 적 충돌 사운드
            clrscr(); // 하트 개수 갱신을 위해 화면 클리어
	    if(life<=0){
		game_over(); // 남은 목숨 없을시 게임오버
	    }
	    init_stage(); // 목숨이 남아 있을시 현재 스테이지 재시작
            return;
        }
    }
    // 코인 충돌 체크
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            coins[i].collected = 1; // 코인상태 -> 먹은것으로 표시
            map[stage][player_y][player_x] = ' '; // map 배열에서 코인 제거
            score += 20; // 점수 증가
            playsound(sound_COIN); // 코인 사운드
        }
    }
}

// Windows 환경
#ifdef _WIN32
    // windows는 즉시 입력 환경이라 Raw 불필요 -> 빈 함수로 처리
    void disable_raw_mode() {}
    void enable_raw_mode() {} 

    // 콘솔 커서 이동 함수
    void gotoxy(int x, int y) {
        // COORD는 0기반 -> x-1, y-1
        COORD pos={(x-1),(y-1)};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    }
    void clrscr() {system("cls");} // 클리어 화면
    void delay(int ms) { Sleep(ms);} // ms 단위 딜레이
    // Windows는 conio.h의 kbhit(), getch() 사용

    // 사운드 : Beep(주파수, 지속시간ms)를 이용
    void playsound(Play type) {
        switch(type) {
            case sound_COIN:
                Beep(1800,50);
                break;
            case sound_ENEMY:
                Beep(400,50);
                Beep(200,50);
                break;
            case sound_CLEAR:
                Beep(1000,100);
                Beep(1200,100);
                Beep(1500,100);
                break;
            case sound_GAMEOVER:
                Beep(400,100);
                Beep(300,100);
                Beep(200,100);
                break;
            default:
                Beep(500,500);
                break;
        }
    }

#else // Linux + macOS

    // 터미널 Raw 모드 비활성화 : 프로그램 종료 시 원래 상태로 복원
    void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }
    // 터미널 Raw 모드 활성화 : 입력 버퍼링/에코 끄고 즉시 키 입력 받을 수 있도록 설정
    void enable_raw_mode() {
        tcgetattr(STDIN_FILENO, &orig_termios); // 현재 설정 저장
        atexit(disable_raw_mode); // 프로그램 종료시 자동 복원
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON); // 에코, ICANON 모드 끄기
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    // 커서 이동
    void gotoxy(int x, int y) {
        printf("\033[%d;%dH",y,x);
        fflush(stdout);
    }
    // 클리어 화면
    void clrscr() {
        printf("\033[2J\033[1;1H");
        fflush(stdout);
    }

    // ms 단위 딜레이 (usleep은 마이크로초 단위 -> ms*1000)
    void delay(int ms) {
        usleep(ms*1000);
    }
    // 비동기 키보드 입력 확인 (non-blocking 키 입력 확인)
    int kbhit() {
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); //ICANON, ECHO 끄기
        tcsetattr(STDIN_FILENO, TCSANOW, &newt); // 새 설정 적용
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0); // 기존 플래그 저장
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); // 논블로킹 설정
        ch = getchar(); // 입력 확인
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 플래그 복원
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if(ch != EOF) {
            // 실제 입력이 있으면 다시 입력 버퍼에 넣기
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    }

    // getch() 문자 입력 : 엔터 없이 한글자 입력
    int getch(void) {
        struct termios oldattr, newattr;
        int ch;
        tcgetattr(STDIN_FILENO, &oldattr);
        newattr = oldattr;
        newattr.c_lflag &= ~(ICANON|ECHO); // ICANON, ECHO 끄기
        tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // 원래 설정 복원
        return ch;
    }

    #if defined(__APPLE__)
        // macOS에서는 afplay 명령어 사용
        void playsound(Play type) {
            // system 함수에서 &으로 백그라운드에서 실행
            // > /dev/null 2>&1 불필요한 터미널 출력을 숨김
            switch (type) {
                case sound_COIN:
                    system("afplay /System/Library/Sounds/Ping.aiff > /dev/null 2>&1 &");
                    break;
                case sound_ENEMY:
                    system("afplay /System/Library/Sounds/Basso.aiff > /dev/null 2>&1 &");
                    break;
                case sound_CLEAR:
                    system("afplay /System/Library/Sounds/Hero.aiff > /dev/null 2>&1 &");
                    break;
                case sound_GAMEOVER:
                    system("afplay /System/Library/Sounds/Sosumi.aiff > /dev/null 2>&1 &");
                    break;
                default:
                    printf("\a");
                    break;
        }
    }
    #else
        // '\a' (벨) 사운드 이용
        void playsound(Play type) {
            switch (type) {
                case sound_COIN:
                    printf("\a");
                    break;
                case sound_ENEMY:
                    printf("\a");
                    break;
                case sound_CLEAR:
                    printf("\a"); fflush(stdout); // 소리 즉시 출력 (버퍼 비우기)
                    delay(300);
                    printf("\a"); fflush(stdout);
                    delay(300);
                    printf("\a");
                    break;
                case sound_GAMEOVER:
                    printf("\a"); fflush(stdout); // 소리 즉시 출력 (버퍼 비우기)
                    delay(200);
                    printf("\a"); fflush(stdout);
                    delay(200);
                    printf("\a"); fflush(stdout);
                    delay(200);
                    printf("\a");
                    break; 
                default:
                    printf("\a");
                    break;
            }
            fflush(stdout);
        }
    #endif
#endif // 플랫폼 분기 끝

// 커서 숨기기 : 게임 진행 중 커서가 깜빡이지 않도록 함
void hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

// 커서 다시 보이기 : 프로그램 종료 시 복구
void show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

void free_maps() { // 맵 메모리 해제
    if(map == NULL) return;

    for(int s = 0; s < MAX_STAGES; s++) { // 2중 for문으로 3차원 배열 해제
        for(int y = 0; y < map_height; y++) {
            free(map[s][y]); // x 포인터 해제
        }
        free(map[s]); // y 포인터 해제
    }
    free(map); // 스테이지 포인터 해제
    map = NULL; // 포인터 초기화
}

// 타이틀 시작 화면
void title() {
    clrscr(); // 클리어
    hide_cursor(); // 커서 숨기기
    enable_raw_mode(); // 즉시 키 입력을 위해 raw 모드 활성화

    printf("\n\n");
    printf("      #  # #  # #### #  # ###  ###\n");
    printf("      ## # #  # #    #  # #  #  # \n");
    printf("      # ## #  # # ## #  # ###   # \n");
    printf("      #  # #  # #  # #  # # #   # \n");
    printf("      #  # #### #### #### #  # ###\n");
    
    printf("\n\n");
    printf("      MAP SIZE: %d x %d, STAGES: %d\n\n", map_width, map_height, MAX_STAGES);
    printf("          PRESS ENTER TO START\n");
    printf("              (q to Quit)\n");
    
    // q입력시 게임 종료
    if(getch() == 'q') {
        printf("\n게임을 종료합니다.\n");
        getch();
        clrscr();
        free_maps();
        disable_raw_mode();
        show_cursor();
        exit(0);
    }
    else return; // main에서 init_stage 후 게임 시작
}

// 게임 오버 화면
void game_over() {
    clrscr();
    printf("\n\n");
    printf("----------------------------------------\n");
    printf("                Game Over               \n");
    printf("----------------------------------------\n");
    printf("                            최종점수: %d\n", score);
    printf("      재시작 : ENTER  |  종료 : Q       \n");
    playsound(sound_GAMEOVER);

    int key;
    while(1){
	key = getch();
	if(key == '\r' || key == '\n'){
        // 엔터 입력 : 게임 상태 초기화 후 처음 스테이지부터 재시작
		stage = 0;
		score = 0;
		life = 3;

        free_maps(); // 먹은 코인 초기화를 위해 맵 메모리 해제
		load_maps(); // 맵 다시 로드

		init_stage(); // 스테이지 초기화
		return; // main의 게임루프로 복귀
	}

    // q 입력 시 완전 종료
	if(key == 'q' || key == 'Q'){
		clrscr();
        free_maps();
		disable_raw_mode();
		show_cursor();
		exit(0);
	}
    }
}

// 엔딩 화면
void ending() {
    clrscr();
    printf("\n\n");
    printf("   ##   #    #       ###  #    ###   ##  ### \n");
    printf("  #  #  #    #      #     #    #    #  # #  #\n");
    printf("  ####  #    #      #     #    ###  #### ### \n");
    printf("  #  #  #    #      #     #    #    #  # # # \n");
    printf("  #  #  #### ####    ###  #### ###  #  # #  #\n");
    printf("\n\n");
    printf("               최종 점수: %d\n", score);
    printf("           PRESS ANY KEY TO QUIT\n");   
    playsound(sound_CLEAR);

    // 버퍼에 남은 입력이 있을시 모두 비우기
    while(kbhit()){
        getch();
    }
    
    // 아무키나 입력 받으면 종료
    getch();
    clrscr();
    free_maps();
    disable_raw_mode();
    show_cursor();
    exit(0);
}
