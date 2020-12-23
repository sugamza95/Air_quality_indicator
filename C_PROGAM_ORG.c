//*********************** SECTION m1 ********************
#include <mega128.h>
#include <delay.h>
#include "mydef_ver3.h"      
#include "lcd.h"
#include "htu21d.h"
#include "dust.h"

extern unsigned int ADC_h;
extern unsigned int ADC_l;
extern int ADC_re;
extern float ADC_vol;
extern float ADC_dust;

extern int err_show;

u08 new_key;
int count=0; //key입력검사 반복문용
int sel_flag=0;//for select at first

interrupt [TIM1_COMPA] void timer1_compa_isr(void){ 

    delay_us(900);//회로 특성으로 280us보다 늦게 출력
    ADCSRA |= 0x40;//adc start, single ended, 인터럽트 enable시 처리전까지 빠져나오지않   
    
    while(ADCSRA&(1<<ADSC)){}   
   
    ADC_l=ADCL;
    ADC_h=ADCH;
    ADC_re=(ADC_h<<2)|(ADC_l>>6);
    
    ADC_vol=((float)ADC_re/1024)*5;
    ADC_dust=(ADC_vol-0.233)/0.005;
    //compase match rise, fall? pulse가 좁아서 가능한건지
        
}

void beep(void){
    BUZ_RLY = BUZ_RLY | BUZZER_ON; 
    delay_ms(100);
    BUZ_RLY = BUZ_RLY & BUZZER_OFF;       
}
 
//-----------------------------------------------

void Timer_init(void){
    TCCR1A=0xc2;// inverted phase correct PWM MODE 8bit COM1A1=1, COM1A0=1,WGM11=1 
    OCR1A=220 ;
    ICR1=227;
    TCCR1B=0x14;//prescale=clk/256 WGM13=1  CS12=1
    TIMSK=0x10;
}  


//----------------------------------------------- 

void key_check(int active){
    while(count!=400){
        new_key = KEY_IN & KEY_MASK; 
        if(active==0){if((new_key!=0b00001111)&&(new_key!=0b00001111)){sel_flag++; break;} }
        if(active==1){if((new_key!=0b00001110)&&(new_key!=0b00001111)){break;}}
        if(active==2){if((new_key!=0b00001101)&&(new_key!=0b00001111)){break;}}
        if(active==3){if((new_key!=0b00001011)&&(new_key!=0b00001111)){break;}}
        if(active==4){if((new_key!=0b00000111)&&(new_key!=0b00001111)){break;}}
        //&&(new_key!=0b00001111)조건이 없다면 스위치를 눌렀다가 떼었을때 mode전환후 0b00001111로 바뀌어버리기 때문에 
        //LCD가 충분한 delay를 가지지 못하고 빨리변한다.  
        //다바운스를 막기위함
        //채터링때문에 못읽는 것이기 때문에
        //여유시간 400ms의 여유시간을 두고 읽는다.
        //단 단순히 delay를 500ms를 주게되면 사용자가 스위치를 눌렀다가 떼었을때 변화가 없을 수 있는데
        //이런 경우를 최대한 막고자 500ms속에서 1ms마다 키를 계속 입력받아 상태를 본다.
        //500ms후에 한번보는 것이랑 500ms속에서 계속 키를검사하는것이랑 정상적으로 동작할 확률이 확연히 다르다.
        delay_ms(1); 
        count++;
    } 
    count=0;                     
}

void main(void)
{

//*********************** SECTION m2 ******************** 
    
    int cir_flag;//순환
//*********************** SECTION m6 ********************
    DDRA = 0x00;
    PORTA = 0x00;      //LCD DATA PORT
    
    DDRB = 0x20;        //timer, PB1,2,3,4 switch
    PORTB = 0x2f;      //SW1,2,3,4 PULL UP ON 내부적으로 풀업저항 설정
    
    DDRC = 0xff;
    PORTC =0xff;       //DEBUG PORT    
    
    DDRE = 0x00;
    PORTE =0x00 | (0x03 << 6);
    
    DDRG = 0xFF;
    PORTG = 0x00;      //BUZZER AND RELAY CONTROL PORT            
//-----------------------------------------------   
    Timer_init(); 
    TWI_initialize();
    ADMUX =0x60; // clk/128, 0 channel, adc enable , left adjust
    ADCSRA =0x80;
 
    lcd_init();                              
//-----------------------------------------------
    lcd_clear();
    lcd_home();

    lcd_control_write(0x80);
    lcd_print_data("AIR_QUAL",8);
    
    lcd_control_write(0xC0);        
    lcd_print_data("ITY",3);    
    delay_ms(3000);    

    lcd_home();    
    lcd_clear(); 
//--------------------------------------------------    
    lcd_control_write(0x80);
    lcd_print_data("INDICATO",8); 
     
    lcd_control_write(0xC0);
    lcd_print_data("R",1);
    delay_ms(3000);
    
    lcd_home();    
    lcd_clear(); 
    
    lcd_control_write(0x80);
    lcd_print_data("VER2.0",6);
    delay_ms(3000);
//---------------------------------------------------
    #asm("sei")
//---------------------------------------------------       
    while (1){ 
      
      while(sel_flag!=1){
       
        lcd_home();    
        lcd_clear();  
        lcd_control_write(0x80);
        lcd_print_data("SELECT",6);
        I2C_HTU21D(0x80,0xE3);  
        
        key_check(0);
        
      } 
         
        switch(new_key){
    
            case TEM: 
                err_show=1;
                beep();
                while(1){
                    I2C_HTU21D(0x80,0xE3);
                    key_check(1);                  
                    if((new_key==0b00001101)||(new_key==0b00001011)||(new_key==0b00000111)){break;}
                }               
                break;   
                            
            case HUM: 
                err_show=1;
                beep(); 
                while(1){  
                    I2C_HTU21D(0x80, 0xE5);
                    key_check(2); 
                    if((new_key==0b00001110)||(new_key==0b00001011)||(new_key==0b00000111)){break;}
                } 
                break;
                   
            case DUST:
                err_show=0;
                beep(); 
                while(1){  
                    I2C_HTU21D(0x80, 0xE3);
                    dust_sensor();
                    key_check(3);                 
                    if((new_key==0b00000111)||(new_key==0b00001110)||(new_key==0b00001101)){break;}
                }      
                break;         
                  
            case CAL: 
                beep();
               
                while(1){
                    cir_flag=0;
                    while(1){
                        err_show=1;
                        I2C_HTU21D(0x80,0xE3);
                        key_check(4);
                        cir_flag++;
                        if(cir_flag==4){cir_flag=0; break;}
                        if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;} 
                    }
                    if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;}
                    //해당 조건문은 키 입력후 break했을때 빠져나오기위함
                             
                    while(1){  
                        err_show=1;
                        I2C_HTU21D(0x80, 0xE5); 
                        key_check(4);                     
                        cir_flag++;
                        if(cir_flag==4){cir_flag=0; break;}
                        if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;}
                    }          
                    if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;}
        
                          
                    while(1){ 
                        err_show=0; 
                        I2C_HTU21D(0x80, 0xE3);
                        dust_sensor(); 
                        key_check(4);
                        cir_flag++;
                        if(cir_flag==4){cir_flag=0; break;}
                        if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;} 
                    }
                         
                    if((new_key==0b00001110)||(new_key==0b00001101)||(new_key==0b00001011)){break;}   
                    
                }                                                                     
                break;
                  
        }
    }   
}
