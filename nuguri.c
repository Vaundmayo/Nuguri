// 크로스 플랫폼 호환을 위한 공통 헤더
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Windows 환경
#ifdef _WIN32
    #include <windows.h> // Sleep(), system("cls") 등 사용
    #include <conio.h> // getch(), kbhit() 등 사용
// Linux 또는 macOS 환경
#else
    #include <unistd.h> // usleep(), read() 등 사용
    #include <termios.h> // 터미널 속성 제어(버퍼링/에코 비활성화 등)
    #include <fcntl.h> // 논블로킹 입력 등
#endif // 운영체제 분기 종료

// 맵 및 게임 요소 정의 (수정된 부분)
#define MAX_ENEMIES 15 // 최대 적 개수 증가
#define MAX_COINS 30   // 최대 코인 개수 증가

// 구조체 정의
typedef struct {
    int x, y;
    int dir; // 1: right, -1: left
} Enemy;

typedef struct {
    int x, y;
    int collected;
} Coin;

typedef enum {
    sound_JUMP,
    sound_COIN,
    sound_ENEMY,
    sound_CLEAR,
    sound_GAMEOVER
} Play;

// 전역 변수
char ***map; // 3차원 배열 포인터 (stage, y, x)
int map_width = 0; // 맵,스테이지 크기 전역 변수 선언
int map_height = 0;
int MAX_STAGES = 0;
int limit_width = 256;
int limit_height = 256;
int player_x, player_y;
int stage = 0;
int score = 0;
int life = 3;

// 플레이어 상태
int is_jumping = 0;
int velocity_y = 0;
int on_ladder = 0;

// 게임 객체
Enemy enemies[MAX_ENEMIES];
int enemy_count = 0;
Coin coins[MAX_COINS];
int coin_count = 0;

#ifndef _WIN32
    // 터미널 설정
    struct termios orig_termios;
#endif

// 함수 선언
void disable_raw_mode();
void enable_raw_mode();
void load_maps();
void init_stage();
void draw_game();
void update_game(char input);
void move_player(char input);
void move_enemies();
void check_collisions();
int kbhit();
void clrscr();
void delay(int ms);
int getch(void);
void gotoxy(int x, int y);
void hide_cursor();
void show_cursor();
void title();
void game_over();
void ending();
void playsound(Play type);
void free_maps();

int main() {
    // Windows 콘솔을 UTF-8 모드로 설정 : 한글 깨짐 방지
    #ifdef _WIN32
        SetConsoleOutputCP(65001); // UTF-8 출력
        SetConsoleCP(65001); // UTF-8 입력
    #endif
    srand(time(NULL));
    load_maps();
    title();
    init_stage();

    char c = '\0';
    int game_over = 0;

    while (!game_over && stage < MAX_STAGES) {
        if (kbhit()) {
            while(kbhit()) { // 버퍼 비우기
	            c = getch();

	#ifdef _WIN32
	    // Windows 방향키 입력 처리
	    if (c == 0 || c == -32 || c == 224) { //224 : unsigned, -32 : signed char
	        int arrow = getch();
	        switch (arrow) {
	            case 72: c = 'w'; break;
	            case 80: c = 's'; break;
	            case 75: c = 'a'; break;
	            case 77: c = 'd'; break;
	        }
	    }
	#else
	    // Linux 방향키 처리
	    if (c == '\033') {
	        getch(); // '['
	        switch (getch()) {
	            case 'A': c = 'w'; break;
	            case 'B': c = 's'; break;
	            case 'C': c = 'd'; break;
	            case 'D': c = 'a'; break;
	        }
	    }
	#endif
	        if (c == 'q') {
	            game_over = 1;
	            continue;
	        }
        }
	}
	
	else {
            c = '\0';
        }

        update_game(c);
        draw_game();
        delay(90);

        if (map[stage][player_y][player_x] == 'E') {
            stage++;
            score += 100;
            playsound(sound_CLEAR);
            if (stage < MAX_STAGES) {
                init_stage();
            } else {
                game_over = 1;
                ending();
            }
        }
    }


    free_maps();
    disable_raw_mode();
    show_cursor();
    return 0;
}

// 맵 파일 로드
void load_maps() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
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

    MAX_STAGES = stage_count; // 스테이지 수 갱신
    map_width = max_width;
    map_height = max_height;

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
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S') {
                player_x = x;
                player_y = y;
            } else if (cell == 'X' && enemy_count < MAX_ENEMIES) {
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1};
                enemy_count++;
            } else if (cell == 'C' && coin_count < MAX_COINS) {
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
    printf("Stage: %d | Score: %d\n", stage + 1, score);
    printf("Life :%d ", life);
    for(int i=0; i<life; i++) printf("❤"); // 남은 생명만큼 하트 출력
    printf("\n조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");

    char display_map[map_height][map_width + 1];
    for(int y=0; y < map_height; y++) {
        for(int x=0; x < map_width; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C') {
                display_map[y][x] = ' ';
            } else {
                display_map[y][x] = cell;
            }
        }
    }
    
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected) {
            display_map[coins[i].y][coins[i].x] = 'C';
        }
    }

    for (int i = 0; i < enemy_count; i++) {
        display_map[enemies[i].y][enemies[i].x] = 'X';
    }

    display_map[player_y][player_x] = 'P';

    for (int y = 0; y < map_height; y++) {
        for(int x=0; x< map_width; x++){
            printf("%c", display_map[y][x]);
        }
        printf("\n");
    }
}

// 게임 상태 업데이트
void update_game(char input) {
    move_player(input);
    move_enemies();
    check_collisions();
}

// 플레이어 이동 로직
void move_player(char input) {
    int before_x = player_x; // 이동 전 위치 저장
    int next_x = player_x, next_y = player_y;

    char floor_tile = (player_y + 1 < map_height) ? map[stage][player_y + 1][player_x] : '#';
    char current_tile = map[stage][player_y][player_x];

    on_ladder = (current_tile == 'H');

    switch (input) {
        case 'a': next_x--; break;
        case 'd': next_x++; break;
        case 'w': if (on_ladder) next_y--; break;
        case 's': if (on_ladder && (player_y + 1 < map_height) && map[stage][player_y + 1][player_x] != '#') next_y++; break;
        case ' ':
            if (!is_jumping && (floor_tile == '#' || on_ladder)) {
                is_jumping = 1;
                velocity_y = -2;
                playsound(sound_JUMP);
            }
            break;
    }

    if (next_x >= 0 && next_x < map_width && map[stage][player_y][next_x] != '#') player_x = next_x;

    if(input == 's' && player_y +2 < map_height && map[stage][player_y + 2][player_x] == 'H' && map[stage][player_y + 1][player_x] != 'H') { // 사다리 내려가기 구현
        next_y = player_y + 2; // 바닥 밑 사다리가 존재하면 2칸 아래로 이동
        player_y = next_y;
        is_jumping = 0;
        velocity_y = 0;
        return;
    }
    
    if (on_ladder && (input == 'w' || input == 's')) {
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
    
    if (player_y >= map_height) init_stage();

    // 벽 끼임 확인 -> x,y 되돌리기
    if (player_x >= 0 && player_x < map_width && 
        player_y >= 0 && player_y < map_height &&
        map[stage][player_y][player_x] == '#') {
        player_x = before_x;
    }
}


// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= map_width || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < map_height && map[stage][enemies[i].y + 1][next_x] == ' ')) {
            enemies[i].dir *= -1;
        } else {
            enemies[i].x = next_x;
        }
    }
}

// 충돌 감지 로직
void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
        if (player_x == enemies[i].x && player_y == enemies[i].y) {
            life--;
            playsound(sound_ENEMY);
            clrscr(); // 하트 개수 갱신을 위해 화면 클리어
	    if(life<=0){
		game_over();
	    }
	    init_stage();
            return;
        }
    }
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            coins[i].collected = 1;
            map[stage][player_y][player_x] = ' '; // map 배열에서 코인 제거
            score += 20;
            playsound(sound_COIN);
        }
    }
}

// Windows 환경
#ifdef _WIN32
    // windows는 즉시 입력 환경이라 Raw 불필요 -> 빈 함수로 처리
    void disable_raw_mode() {}
    void enable_raw_mode() {} 

    void gotoxy(int x, int y) {
        COORD pos={(x-1),(y-1)};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    }
    void clrscr() {system("cls");} // 클리어 화면
    void delay(int ms) { Sleep(ms);} // ms 단위 딜레이
    // Windows는 conio.h의 kbhit(), getch() 사용

    void playsound(Play type) {
        switch(type) {
            case sound_JUMP:
                Beep(800,50);
                break;
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

    // 터미널 Raw 모드 활성화/비활성화
    void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }
    void enable_raw_mode() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        atexit(disable_raw_mode);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

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
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    }

    // getch() 문자 입력
    int getch(void) {
        struct termios oldattr, newattr;
        int ch;
        tcgetattr(STDIN_FILENO, &oldattr);
        newattr = oldattr;
        newattr.c_lflag &= ~(ICANON|ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
        return ch;
    }

    #if defined(__APPLE__)
        // macOS에서는 afplay 명령어 사용
        void playsound(Play type) {
            // system 함수에서 &으로 백그라운드에서 실행
            // > /dev/null 2>&1 불필요한 터미널 출력을 숨김
            switch (type) {
                case sound_JUMP:
                    system("afplay /System/Library/Sounds/Tink.aiff > /dev/null 2>&1 &");
                    break;
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
        void playsound(Play type) {
            switch (type) {
                case sound_JUMP:
                    printf("\a");
                    break;
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
#endif

// 커서 숨기기
void hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

// 커서 다시 보이기
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

void title() {
    clrscr();
    hide_cursor();
    enable_raw_mode();

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

    if(getch() == 'q') {
        printf("\n게임을 종료합니다.\n");
        getch();
        clrscr();
        free_maps();
        disable_raw_mode();
        show_cursor();
        exit(0);
    }
    else return;
}

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
		stage = 0;
		score = 0;
		life = 3;

        free_maps(); // 먹은 코인 초기화를 위해 맵 메모리 해제
		load_maps(); // 맵 다시 로드

		init_stage();
		return;
	}

	if(key == 'q' || key == 'Q'){
		clrscr();
        free_maps();
		disable_raw_mode();
		show_cursor();
		exit(0);
	}
    }
}

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
    while(kbhit()){
        getch();
    }
    
    getch();
    clrscr();
    free_maps();
    disable_raw_mode();
    show_cursor();
    exit(0);
}
