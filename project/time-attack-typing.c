#include <mega128.h>
#include <delay.h>
#include <stdlib.h>

// 기존 정의 유지
#define ENABLE PORTA.2
#define FUNCSET 0x28
#define ENTMODE 0x06
#define ALLCLR 0x01
#define DISPON 0x0c
#define LINE2 0xC0

typedef unsigned char u_char;
typedef unsigned char lcd_char;

// 무작위 키 패턴 정의
flash char key_options[4] = { 'A', 'S', 'D', 'W' };
flash u_char seg_pat[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };

u_char sec_up = 0, sec_low = 100;  // 분/초
u_char time_limits[5] = { 30, 25, 20, 15, 10 };  // 각 라운드 제한 시간
bit timer_running = 0;  // 타이머 상태
bit game_running = 0;   // 게임 상태
char random_keys[5];  // 라운드별 패턴
char user_input[5];
u_char input_index = 0;
u_char round = 1;  // 현재 라운드 (1부터 시작)

// 함수 선언
void Time_out(void);
void LCD_init(void);
void LCD_String(char flash*);
void Busy(void);
void Command(lcd_char);
void Data(lcd_char);
void GenerateRandomKeys(void);
void DisplayPattern(void);
void CheckUserInput(char);
void NextRound(void);
void GameOver(void);
void ResetGame(void);
void USART_Init(unsigned int);
char USART_Receive(void);
void USART_Transmit(char);

// 메인 함수
void main(void) {
    DDRB = 0xF0;
    DDRD = 0xF0;
    DDRG = 0x0F;

    EIMSK = 0b00110000;
    EICRB = 0b00001000;
    SREG = 0x80;

    LCD_init();
    LCD_String("Time Attack Typing");  // 게임 제목
    Command(LINE2);                    // 두 번째 줄로 이동

    while (1) {
        LCD_String("Press KEY1 to Start");  // 'Press KEY1 to Start' 표시
        delay_ms(500);                     // 500ms 대기

        Command(ALLCLR);                   // 화면 지우기
        delay_ms(500);                     // 500ms 대기
    }

    USART_Init(103);  // 9600bps

    while (1) {
        if (timer_running) {
            Time_out();

            // USART 입력 처리
            if (game_running && (UCSR0A & (1 << RXC0))) {
                char received_char = USART_Receive();
                CheckUserInput(received_char);
                USART_Transmit(received_char);
            }

            // 제한 시간 감소
            sec_low -= 1;
            if (sec_low == 0) {
                sec_low = 100;
                if (sec_up > 0) {
                    sec_up -= 1;
                }
                else {
                    GameOver();  // 시간 초과 시 게임 종료
                }
            }
        }
    }
}

// USART 초기화
void USART_Init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_Transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

char USART_Receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

// 패턴 생성
void GenerateRandomKeys(void) {
    int i;  // i를 int로 선언
    for (i = 0; i < 5; i++) {
        random_keys[i] = key_options[rand() % 4];  // rand() 사용
    }
}

// LCD에 패턴 표시
void DisplayPattern(void) {
    int i;  // 변수 선언을 for문 밖에서 먼저 선언
    Command(ALLCLR);
    Command(LINE2);

    for (i = 0; i < 5; i++) {
        Data(random_keys[i]);  // random_keys 배열의 각 문자 출력
    }
}

// 사용자 입력 처리
void CheckUserInput(char received_char) {
    user_input[input_index] = received_char;

    if (user_input[input_index] == random_keys[input_index]) {
        input_index++;
        if (input_index == 5) {
            NextRound();  // 성공 시 다음 라운드로
        }
    }
    else {
        GameOver();  // 실패 시 게임 종료
    }
}

// 라운드 성공 처리
void NextRound(void) {
    round++;
    if (round > 5) {  // 5라운드 클리어
        Command(ALLCLR);
        LCD_String("You Win!");
        timer_running = 0;
        game_running = 0;
    }
    else {
        sec_up = time_limits[round - 1];
        sec_low = 100;
        input_index = 0;
        GenerateRandomKeys();
        DisplayPattern();
    }
}

// 게임 실패 처리
void GameOver(void) {
    Command(ALLCLR);
    LCD_String("Game Over!");
    delay_ms(2000);
    ResetGame();
}

// 게임 재시작
void ResetGame(void) {
    round = 1;
    sec_up = time_limits[0];
    sec_low = 100;
    input_index = 0;
    game_running = 0;
    timer_running = 0;

    Command(ALLCLR);
    LCD_String("Start Press KEY1");
}

// 7세그먼트 표시
void Time_out(void) {
    PORTG = 0b00001000;
    PORTD = ((seg_pat[sec_low % 10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[sec_low % 10] & 0x70) | (PORTB & 0x0F);
    delay_us(2500);
    PORTG = 0b00000100;
    PORTD = ((seg_pat[sec_low / 10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[sec_low / 10] & 0x70) | (PORTB & 0x0F);
    delay_us(2500);
    PORTG = 0b00000010;
    PORTD = ((seg_pat[sec_up % 10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[sec_up % 10] & 0x70) | (PORTB & 0x0F);
    delay_us(2500);
    PORTG = 0b00000001;
    PORTD = ((seg_pat[sec_up / 10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[sec_up / 10] & 0x70) | (PORTB & 0x0F);
    delay_us(2500);
}

// LCD 초기화
void LCD_init(void) {
    DDRA = 0xFF;
    PORTA = 0x00;
    Command(0x20);
    delay_ms(15);
    Command(FUNCSET);
    Command(DISPON);
    Command(ALLCLR);
    Command(ENTMODE);
}

void LCD_String(char flash* str) {
    while (*str) Data(*str++);
}

void Command(lcd_char byte) {
    Busy();
    PORTA = 0x00;
    PORTA |= (byte & 0xF0);
    delay_us(1);
    ENABLE = 1;
    delay_us(1);
    ENABLE = 0;

    PORTA = 0x00;
    PORTA |= (byte << 4);
    delay_us(1);
    ENABLE = 1;
    delay_us(1);
    ENABLE = 0;
}

void Data(lcd_char byte) {
    Busy();
    PORTA = 0x01;
    PORTA |= (byte & 0xF0);
    delay_us(1);
    ENABLE = 1;
    delay_us(1);
    ENABLE = 0;

    PORTA = 0x01;
    PORTA |= (byte << 4);
    delay_us(1);
    ENABLE = 1;
    delay_us(1);
    ENABLE = 0;
}

void Busy(void) {
    delay_ms(2);
}

// 외부 인터럽트로 게임 시작
interrupt[EXT_INT4] void external_int4(void) {
    if (!game_running) {
        timer_running = 1;
        sec_up = time_limits[0];
        sec_low = 100;
        GenerateRandomKeys();
        DisplayPattern();
        game_running = 1;
    }
}

// 인터럽트 초기화
interrupt[EXT_INT5] void external_int5(void) {
    ResetGame();  // 게임 리셋
}
