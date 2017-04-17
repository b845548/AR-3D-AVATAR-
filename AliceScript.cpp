#include<cstdlib>
#include<cstdio>
#include<iostream>
#include<string>




enum{
    STATE_INIT,
    STATE_MAKING,
    STATE_STARTSPEAK,
    STATE_SPEAKING,
};




class AliceScript{
    int state;

  public:
    AliceScript(void);
    int get_state(void);
    void set_state(int);
    void gen_answer(std::string);
    void gen_speak();
    void finish_make(void);
    void finish_speak(void);
    
};

AliceScript::AliceScript(void){
    state=STATE_INIT;    
}

int AliceScript::get_state(void){
    return state;
}
void AliceScript::set_state(int state){
    this->state=state;
}

void AliceScript::gen_answer(std::string question){
    set_state(STATE_MAKING);
    std::string prefix = std::string("(echo -e \"");
    std::string suffix = std::string("\nq\" | java -cp out/production/Ab Main bot=super action=chat trace=false | grep \"Robot\" > result.tmp; exit ) &");
    system((prefix+question+suffix).c_str());   
}


void AliceScript::finish_make(void){
FILE* f;
int count;
f=fopen("result.tmp","r");
if(f==NULL)
    return;
for(count=0;fgetc(f)!=EOF;count++);
fclose(f);
if(count<14)
    return;

set_state(STATE_STARTSPEAK);

}

void AliceScript::gen_speak(void){
FILE* f;
char buff[100];
f=fopen("result.tmp","r");
if(f==NULL)
    return;
if(fgets(buff,100,f)==NULL)
    return;

std::string parse = std::string (buff); 
std::string answer = parse.substr (14,parse.size()-14); 
std::string prefix = std::string("((pico2wave -w test.wav \"");
std::string suffix = std::string("\"; aplay test.wav ; echo a > finish.tmp ); exit) &");

system((prefix+answer+suffix).c_str());

set_state(STATE_SPEAKING);

}

void AliceScript::finish_speak(void){
FILE* f;
f=fopen("finish.tmp","r");
if(f==NULL)
    return;
system("rm finish.tmp");
set_state(STATE_INIT);

}

int main(){
AliceScript as;
std::string question=std::string("hello");
int stop=2;
while(stop){
//std::cout<<as.get_state();
switch(as.get_state()){
    case STATE_INIT:
        stop--;
        as.gen_answer(question);
    break;
    case STATE_MAKING:
        as.finish_make();
    break;
    case STATE_STARTSPEAK:
        as.gen_speak();
    break;
    case STATE_SPEAKING:
        as.finish_speak();
    break;
    default:
    break;
    
}

}
}
