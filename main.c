#include <stm32f031x6.h>
#include "display.h"
#include "musical_notes.h"

void initClock(void);
void initSysTick(void);
void SysTick_Handler(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
volatile uint32_t milliseconds;
void initTimer(void);
void playNote(uint32_t Freq,uint32_t duration);
void eputchar(char c);
void eputs(char *String);
void printDecimal(int32_t Value);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160

#define BW 16
#define BH 9
#define BAT_WIDTH 10
#define MAX_BRICI_LEVELS 4

uint32_t ball_x;
uint32_t ball_y;
uint32_t bat_x;
uint32_t bat_y;

typedef struct  {
    uint16_t colour;
    uint16_t x;
    uint16_t y;    
    uint16_t visible;
} block_t;

#define LTBLUE 0xaedc  // Light Blue
#define MDBLUE 0x001f  // Medium Blue
#define DKBLUE 0x0010  // Dark Blue
#define BLOCKCOUNT 24

block_t Blocks[BLOCKCOUNT] = {
    // Light Blue bricks
    { LTBLUE, 0, 18, 0 }, { LTBLUE, 16, 18, 0 }, { LTBLUE, 32, 18, 0 }, { LTBLUE, 48, 18, 0 },
    { LTBLUE, 64, 18, 0 }, { LTBLUE, 80, 18, 0 }, { LTBLUE, 96, 18, 0 }, { LTBLUE, 112, 18, 0 }, 

    // Medium Blue bricks
    { MDBLUE, 0, 27, 0 }, { MDBLUE, 16, 27, 0 }, { MDBLUE, 32, 27, 0 }, { MDBLUE, 48, 27, 0 },
    { MDBLUE, 64, 27, 0 }, { MDBLUE, 80, 27, 0 }, { MDBLUE, 96, 27, 0 }, { MDBLUE, 112, 27, 0 },

    // Dark Blue bricks
    { DKBLUE, 0, 36, 0 }, { DKBLUE, 16, 36, 0 }, { DKBLUE, 32, 36, 0 }, { DKBLUE, 48, 36, 0 },
    { DKBLUE, 64, 36, 0 }, { DKBLUE, 80, 36, 0 }, { DKBLUE, 96, 36, 0 }, { DKBLUE, 112, 36, 0 },
};



  
void hideBlock(uint32_t index);
void showBlock(uint32_t index);
void hideBall(void);
void showBall(void);
void moveBall(uint32_t newX, uint32_t newY);
void hideBat(void);
void showBat(void);
void moveBat(uint32_t newX, uint32_t newY);
int blockTouching(int Index,uint16_t ball_x,uint16_t ball_y);
int UpPressed(void);
int DownPressed(void);
int LeftPressed(void);
int RightPressed(void);
void randomize(void);
uint32_t random(uint32_t lower, uint32_t upper);
void playBreakie(void);

int main()
    {
    initClock();
    RCC->AHBENR |= (1 << 18) + (1 << 17); // enable Ports A and B
    initSysTick();
    initTimer();
    display_begin();
    pinMode(GPIOB,0,1);
    pinMode(GPIOB,4,0);
    pinMode(GPIOB,5,0);
    pinMode(GPIOA,8,0);
    pinMode(GPIOA,1,1);
    enablePullUp(GPIOB,0);
    enablePullUp(GPIOB,4);
    enablePullUp(GPIOB,5);
    enablePullUp(GPIOA,11);
    enablePullUp(GPIOA,8);

    while(1)
    {
        playBreakie();
    }
}
void playBreakie()
{
    int Level = MAX_BRICI_LEVELS;
    int LevelComplete = 0;
    unsigned int BallCount = 5;
    unsigned int Index;
    int32_t BallXVelocity = 1;
    int32_t BallYVelocity = 1;
    // Blank the screen to start with
    fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    bat_x = 20;
    bat_y = SCREEN_HEIGHT - 20;    
    
    printText("Breakie", 5, 40, RGBToWord(0x03FC, 0x03FC, 0x03FC), RGBToWord(0, 0, 0));
    printText("Press Left <-", 10, 50, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));
    printText("To start", 10, 60, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));
    while(!LeftPressed());
   
    randomize();  
    ball_x = random(0,SCREEN_WIDTH);
    ball_y = random(50,(bat_y - 10));      
    // draw the red squares indicating the number of remaining lives.
    for (Index = BallCount; Index > 0; Index--)
        fillRectangle(SCREEN_WIDTH - Index * 8, SCREEN_HEIGHT-10, 7, 7, RGBToWord(0xff, 0xf, 0xf));
        

    // Boolean to keep track if game is paused/unpaused
    int paused = 0;
    
    while (Level > 0)
    {
        moveBall(random(10, SCREEN_WIDTH-10), SCREEN_HEIGHT/2);
    
        if (random(0,2) > 0) // "flip a coin" to choose ball x velocity
            BallXVelocity = 1;
        else
            BallXVelocity = -1;
        
        LevelComplete = 0;
        BallYVelocity = -1;  // initial ball motion is up the screen
        fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_WIDTH, 0); // clear the screen
        // draw the blocks.
        for (Index=0;Index<BLOCKCOUNT;Index++)
        {
            showBlock(Index);
        }
        showBall();
        showBat();
        printText("Level", 5, SCREEN_HEIGHT-10, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));
        printNumber(MAX_BRICI_LEVELS - Level + 1, 45, SCREEN_HEIGHT-10, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));
        
        while (!LevelComplete)
        {
            int detectDownPress = 0;
            
            // Restart game if UP button pressed
            if (UpPressed())
                return;

            // Check whether to pause game
            if (!detectDownPress && (DownPressed()))
            {
                detectDownPress = 1;
                paused = paused ? 0 : 1;
                while (DownPressed());
            }

            if (paused)
            {
                fillRectangle(58, 60, 5, 15, RGBToWord(0xff, 0xff, 0xff));
				fillRectangle(67, 60, 5, 15, RGBToWord(0xff, 0xff, 0xff));
                continue;
            }
			
			fillRectangle(58, 60, 5, 15, RGBToWord(0x00, 0x00, 0x00));
			fillRectangle(67, 60, 5, 15, RGBToWord(0x00, 0x00, 0x00));

            
            // Handle input from buttons or serial port

            if (RightPressed())
            {
                // Move right
                if (bat_x < (SCREEN_WIDTH - BAT_WIDTH))
                {
                    moveBat(bat_x + 2, bat_y); // Move the bat faster than the ball
                }
            }

            if (LeftPressed()) 
            {
                // Move left
                if (bat_x > 0)
                {
                    moveBat(bat_x - 2, bat_y); // Move the bat faster than the ball
                }
            }
            
            // Ball hit bat
            if ((ball_y == bat_y) && (ball_x >= bat_x) && (ball_x <= bat_x + BAT_WIDTH))
            {
                eputs("Ball hit bat\n\r");
                playNote(1500, 50);
                BallYVelocity = -BallYVelocity;
            }

            showBat(); // redraw bat as it might have lost a pixel due to collisions
            moveBall(ball_x+BallXVelocity,ball_y+BallYVelocity);
            
            // Play beep sound if ball hits walls
            if (ball_x == 2)
            {
                BallXVelocity = -BallXVelocity;
                playNote(1000, 50);
            }
        
            if (ball_x == SCREEN_WIDTH - 2)
            {
                BallXVelocity = -BallXVelocity;
                playNote(1000, 50);
            }
            
            if (ball_y == 2)
            {
                BallYVelocity = -BallYVelocity;
                playNote(1000, 50);
            }

            if (ball_y >= bat_y+3)  // has the ball pass the bat vertically?
            {
                BallCount--;
                
                if (BallCount > 0)
                    playNote(250, 100);
                
                if (BallCount == 0)
                {
                    // Send message to serial port
                    eputs("Game over, level: ");
                    printDecimal(MAX_BRICI_LEVELS - Level + 1);
                    eputs("\n\r");


                    fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
                    printText("GAME OVER", 40, 100, RGBToWord(0xff, 0xff, 0xff), 0);
                    printText("Up to restart", 18, 120, RGBToWord(0xff, 0xff, 0), RGBToWord(0, 0, 0));
                    while (!UpPressed());
                    return;
                }

                if (random(0,100) & 1)
                    BallXVelocity = 1;
                else
                    BallXVelocity = -1;

                BallYVelocity = -1;
                moveBall(random(10, SCREEN_WIDTH - 10), random(90, 120));
                fillRectangle(SCREEN_WIDTH-5*15, SCREEN_HEIGHT-10, 120, 10, 0);
                for (Index = BallCount; Index > 0; Index--)
                    fillRectangle(SCREEN_WIDTH - Index * 8, SCREEN_HEIGHT-10, 7, 7, RGBToWord(0xff, 0xf, 0xf));
            }

            // check for ball hitting blocks and if this level is done.
            LevelComplete = 1;
            for (Index = 0; Index < BLOCKCOUNT; Index++)
            {
                int touch = blockTouching(Index,ball_x,ball_y);
                if (touch)
                {
                    hideBlock(Index);
                    if ( (touch == 1) || (touch == 3) )
                        BallYVelocity = -BallYVelocity;
                    if ( (touch == 2) || (touch == 4) )
                        BallXVelocity = -BallXVelocity;

                    // Also print block number that was hit
                    eputs("\r\nBlock ");
                    printDecimal(Index);
                    eputs(" hit\n\r");
                    
                    playNote(1500, 50);
                }
                
                if (Blocks[Index].visible) // any blocks left?
                    LevelComplete = 0;
            }

            // No Blocks left, Move to next level.
            if ((LevelComplete == 1) && (Level > 0))
            {
                Level--;
                
                // Print new level
                eputs("Level Up: ");
                printDecimal(MAX_BRICI_LEVELS - Level + 1);
                eputs("\n\r");
        
                printText("Level",5, SCREEN_HEIGHT-10, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));
                printNumber(MAX_BRICI_LEVELS - Level + 1, 45, SCREEN_HEIGHT-10, RGBToWord(0xff, 0xff, 0xff), RGBToWord(0, 0, 0));

            }

            delay(10+Level*5); // Slow the game to human speed and make it level dependant.
        }
    }

    fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_WIDTH, RGBToWord(0, 0, 0xff));
    printText("VICTORY!",40, 100, RGBToWord(0xff, 0xff, 0), RGBToWord(0, 0, 0xff));
    printText("Up to restart", 18, 120, RGBToWord(0xff, 0xff, 0), RGBToWord(0, 0, 0xff));
    while (!UpPressed());
    return;
}


void initSysTick(void)
{
    SysTick->LOAD = 48000;
    SysTick->CTRL = 7;
    SysTick->VAL = 10;
    __asm(" cpsie i "); // enable interrupts
}
void SysTick_Handler(void)
{
    milliseconds++;
}
void initClock(void)
{
    // This is potentially a dangerous function as it could
    // result in a system with an invalid clock signal - result: a stuck system
    // Set the PLL up
    // First ensure PLL is disabled
    RCC->CR &= ~(1u<<24);
    while( (RCC->CR & (1 <<25))); // wait for PLL ready to be cleared
    
    // Warning here: if system clock is greater than 24MHz then wait-state(s) need to be
    // inserted into Flash memory interface
                
    FLASH->ACR |= (1 << 0);
    FLASH->ACR &=~((1u << 2) | (1u<<1));
    // Turn on FLASH prefetch buffer
    FLASH->ACR |= (1 << 4);
    // set PLL multiplier to 12 (yielding 48MHz)
    RCC->CFGR &= ~((1u<<21) | (1u<<20) | (1u<<19) | (1u<<18));
    RCC->CFGR |= ((1<<21) | (1<<19) ); 

    // Need to limit ADC clock to below 14MHz so will change ADC prescaler to 4
    RCC->CFGR |= (1<<14);

    // and turn the PLL back on again
    RCC->CR |= (1<<24);        
    // set PLL as system clock source 
    RCC->CFGR |= (1<<1);
}
void delay(volatile uint32_t dly)
{
    uint32_t end_time = dly + milliseconds;
    while(milliseconds != end_time)
    __asm(" wfi "); // sleep
}

void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
    Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
    Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
    uint32_t mode_value = Port->MODER;
    Mode = Mode << (2 * BitNumber);
    mode_value = mode_value & ~(3u << (BitNumber * 2));
    mode_value = mode_value | Mode;
    Port->MODER = mode_value;
}
void initTimer()
{
    // Power up the timer module
    RCC->APB1ENR |= (1 << 8);
    pinMode(GPIOB,1,2); // Assign a non-GPIO (alternate) function to GPIOB bit 1
    GPIOB->AFR[0] &= ~(0x0fu << 4); // Assign alternate function 0 to GPIOB 1 (Timer 14 channel 1)
    TIM14->CR1 = 0; // Set Timer 14 to default values
    TIM14->CCMR1 = (1 << 6) + (1 << 5);
    TIM14->CCER |= (1 << 0);
    TIM14->PSC = 48000000UL/65536UL; // yields maximum frequency of 21kHz when ARR = 2;
    TIM14->ARR = (48000000UL/(uint32_t)(TIM14->PSC))/((uint32_t)440);
    TIM14->CCR1 = TIM14->ARR/2;    
    TIM14->CNT = 0;
}
void playNote(uint32_t Freq,uint32_t duration)
{
    TIM14->CR1 = 0; // Set Timer 14 to default values
    TIM14->CCMR1 = (1 << 6) + (1 << 5);
    TIM14->CCER |= (1 << 0);
    TIM14->PSC = 48000000UL/65536UL; // yields maximum frequency of 21kHz when ARR = 2;
    TIM14->ARR = (48000000UL/(uint32_t)(TIM14->PSC))/((uint32_t)Freq);
    TIM14->CCR1 = TIM14->ARR/2;    
    TIM14->CNT = 0;
    TIM14->CR1 |= (1 << 0);
    uint32_t end_time=milliseconds+duration;
    while(milliseconds < end_time);
    TIM14->CR1 &= ~(1u << 0);
}

int UpPressed(void)
{
    if ((GPIOA->IDR & (1<<8)) == 0)
        return 1;
    else
        return 0;
}
int DownPressed(void)
{
    if ((GPIOA->IDR & (1<<11)) == 0)
        return 1;
    else
        return 0;
}
int LeftPressed(void)
{
    if ( (GPIOB->IDR & (1<<5)) == 0)
        return 1;
    else
        return 0;
}
int RightPressed(void)
{
    if ( (GPIOB->IDR & (1<<4)) == 0)
        return 1;
    else
        return 0;
}


void hideBlock(uint32_t index)
{
    fillRectangle(Blocks[index].x,Blocks[index].y,BW,BH,0);
    Blocks[index].visible = 0;
}
void showBlock(uint32_t index)
{    
    fillRectangle(Blocks[index].x,Blocks[index].y,BW,BH,Blocks[index].colour);
    Blocks[index].visible = 1;
}
void hideBall(void)
{
    fillRectangle(ball_x,ball_y,2,2,0);
}
void showBall(void)
{
	static float i = 100;
	i += 15;
	if(i > 200)
	{
		i = 0;
	}
    fillRectangle(ball_x,ball_y,2,2,RGBToWord(255, (int)i, 255-(int)i));
}
void moveBall(uint32_t newX, uint32_t newY)
{
    hideBall();
    ball_x = newX;
    ball_y = newY;
    showBall();
}
void hideBat(void)
{
    fillRectangle(bat_x,bat_y,10,3,0);
}
void showBat(void)
{
	static float i = 50;
	i += 10;
	if(i > 170)
	{
		i = 0;
	}
    fillRectangle(bat_x,bat_y,10,3,RGBToWord((int) i, (int)i, 255-(int)i));
}    
void moveBat(uint32_t newX, uint32_t newY)
{
    hideBat();
    bat_x = newX;
    bat_y = newY;
    showBat();
}
int blockTouching(int Index,uint16_t x,uint16_t y)
{
    
    // This function returns a non zero value if the object at x,y touches the sprite
    // The sprite is assumed to be rectangular and returns a value as follows:
    // 0 : not hit
    // 1 : touching on top face (lesser Y value)
    // 2 : touching on left face (lesser X value)
    // 3 : touching on bottom face (greater Y value)    
    // 4 : touching on right face (greater X value)
    if (Blocks[Index].visible == 0)
        return 0;
    if ( Blocks[Index].y == ball_y  )
    {  // top face?
      if ( (x>=Blocks[Index].x) && ( x < (Blocks[Index].x+BW) ) )
        return 1;      
    }
    if ( x == Blocks[Index].x )
    {
      // left face
      if ( (y>=Blocks[Index].y) && ( y < (Blocks[Index].y+BH) ) )
        return 2;
    }
    if ( y == (Blocks[Index].y+BH-1)  )
    {  // bottom face?
      if ( (x>=Blocks[Index].x) && ( x < (Blocks[Index].x+BW) ) )
        return 3;      
    }
    if ( x == (Blocks[Index].x + BW-1) )
    {
      // right face
      if ( (y>=Blocks[Index].y) && ( y < (Blocks[Index].y+BH) ) )
        return 4;
    }

    return 0; // not touching
}
static uint32_t prbs_shift_register=0;
void randomize(void)
{
    while(prbs_shift_register ==0) // can't have a value of zero here
        prbs_shift_register=milliseconds;
    
}
uint32_t random(uint32_t lower, uint32_t upper)
{
    uint32_t new_bit=0;    
    uint32_t return_value;
    new_bit= ((prbs_shift_register & (1<<27))>>27) ^ ((prbs_shift_register & (1<<30))>>30);
    new_bit= ~new_bit;
    new_bit = new_bit & 1;
    prbs_shift_register=prbs_shift_register << 1;
    prbs_shift_register=prbs_shift_register | (new_bit);
    return_value = prbs_shift_register;
    return_value = (return_value)%(upper-lower)+lower;
    return return_value;
}
void eputchar(char c)
{
    while( (USART1->ISR & (1 << 6)) == 0); // wait for any ongoing
    USART1->ICR=0xffffffff;
    // transmission to finish
    USART1->TDR = c;
}
void eputs(char *String)
{
    while(*String) // keep printing until a NULL is found
    {
        eputchar(*String);
        String++;
    }
}
void printDecimal(int32_t Value)
{
    char DecimalString[12]; // a 32 bit value range from -2 billion to +2 billion approx
                            // That's 10 digits
                            // plus a null character, plus a sign
    DecimalString[11] = 0;  // terminate the string;

    if (Value < 0)
        Value = -Value;
    
    int index = 10;
    while(index > 0)
    {
        DecimalString[index]=(Value % 10) + '0';
        Value = Value / 10;
        
        if (Value == 0)
            break;
        
        index--;
    }
    eputs(&DecimalString[index]);
}