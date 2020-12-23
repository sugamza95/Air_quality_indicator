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
int count=0; //key�Է°˻� �ݺ�����
int sel_flag=0;//for select at first

interrupt [TIM1_COMPA] void timer1_compa_isr(void){ 

    delay_us(900);//ȸ�� Ư������ 280us���� �ʰ� ���
    ADCSRA |= 0x40;//adc start, single ended, ���ͷ�Ʈ enable�� ó�������� ������������   
    
    while(ADCSRA&(1<<ADSC)){}   
   
    ADC_l=ADCL;
    ADC_h=ADCH;
    ADC_re=(ADC_h<<2)|(ADC_l>>6);
    
    ADC_vol=((float)ADC_re/1024)*5;
    ADC_dust=(ADC_vol-0.233)/0.005;
    //compase match rise, fall? pulse�� ���Ƽ� �����Ѱ���
        
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
        //&&(new_key!=0b00001111)������ ���ٸ� ����ġ�� �����ٰ� �������� mode��ȯ�� 0b00001111�� �ٲ������� ������ 
        //LCD�� ����� delay�� ������ ���ϰ� �������Ѵ�.  
        //�ٹٿ�� ��������
        //ä�͸������� ���д� ���̱� ������
        //�����ð� 400ms�� �����ð��� �ΰ� �д´�.
        //�� �ܼ��� delay�� 500ms�� �ְԵǸ� ����ڰ� ����ġ�� �����ٰ� �������� ��ȭ�� ���� �� �ִµ�
        //�̷� ��츦 �ִ��� ������ 500ms�ӿ��� 1ms���� Ű�� ��� �Է¹޾� ���¸� ����.
        //500ms�Ŀ� �ѹ����� ���̶� 500ms�ӿ��� ��� Ű���˻��ϴ°��̶� ���������� ������ Ȯ���� Ȯ���� �ٸ���.
        delay_ms(1); 
        count++;
    } 
    count=0;                     
}

void main(void)
{

//*********************** SECTION m2 ******************** 
    
    int cir_flag;//��ȯ
//*********************** SECTION m6 ********************
    DDRA = 0x00;
    PORTA = 0x00;      //LCD DATA PORT
    
    DDRB = 0x20;        //timer, PB1,2,3,4 switch
    PORTB = 0x2f;      //SW1,2,3,4 PULL UP ON ���������� Ǯ������ ����
    
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
                    //�ش� ���ǹ��� Ű �Է��� break������ ��������������
                             
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
