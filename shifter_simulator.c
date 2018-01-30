#include "neslib.h"
#include "shifter_simulator.h"

#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_UP 3
#define DIR_DOWN 4

#define MAX_COL  3
#define MAX_LIN  3

#define MAX_REV 9999
#define RED_ZONE 9000
#define RALENTI 1000
#define ACCEL 50
#define DECCEL 200

const unsigned char metaShifter[]={
	0,	0,	0x45,	0,
	8,	0,	0x46,	0,
	16,	0,	0x47,	0,
	0,	8,	0x48,	0,
	8,	8,	0x49,	0,
	16,	8,	0x4A,	0,
	0,	16,	0x4B,	0,
	8,	16,	0x4C,	0,
	16,	16,	0x4D,	0,
	128
};
const unsigned char fuckShifter[]={
	0,	0,	0x00,	0,
	8,	0,	0x00,	0,
	16,	0,	0x00,	0,
	0,	8,	0x00,	0,
	8,	8,	0x00,	0,
	16,	8,	0x00,	0,
	0,	16,	0x00,	0,
	8,	16,	0x00,	0,
	16,	16,	0x00,	0,
	128
};

const unsigned char palette[16]={ 0x0f,0x26,0x16,0x38,0x0f,0x01,0x21,0x31,0x0f,0x06,0x16,0x26,0x0f,0x09,0x19,0x29 };
const unsigned int ratio[7]={0, 200, 150, 100, 75, 60, 40};
const unsigned int min_rev[7]={0, 0, 2500, 2500, 2500, 2500, 2500};
static unsigned char list[3*12+1];
static unsigned char game_state, sound_state, input_state;
static unsigned char pad, spr, state_sprite, dir_sprite;
static unsigned char shifter_x, shifter_y, wait;
static unsigned int rev, speed, win, loose;
static unsigned	int shifterPosX, shifterPosY, i, j;
static int boite[3][3];


void put_str(unsigned int adr,const char *str)
{
	ppu_off();
	vram_adr(adr);

	while(1)
	{
		if(!*str)
		{
			ppu_on_all();
			break;
		}
		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void put_str_up(int horz, const int vert, const char *str)
{
	i=0;
	while(1)
	{
		if(!*str)break;
		list[i]= MSB(NTADR_A(horz,vert));
		i++;
		list[i]= LSB(NTADR_A(horz,vert));
		i++;
		list[i]=(*str++)-0x20;
		i++;
		horz++;
		//vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void put_int(const int num,int horz, const int vert, const int sco)
{
	/*
	ppu_off();
	vram_adr(adr);
	vram_put((sco/1000)+0x10);
	vram_put((sco/100)+0x10);
	vram_put((sco/10)+0x10);
	vram_put((sco%10)+0x10);
	ppu_on_all();
	*/
	list[0+12*num]= MSB(NTADR_A(horz,vert));
	list[1+12*num]= LSB(NTADR_A(horz,vert));
	list[2+12*num]=(sco/1000)%10+0x10;
	horz++;
	list[3+12*num]= MSB(NTADR_A(horz,vert));
	list[4+12*num]= LSB(NTADR_A(horz,vert));
	list[5+12*num]=(sco/100)%10+0x10;
	horz++;
	list[6+12*num]= MSB(NTADR_A(horz,vert));
	list[7+12*num]= LSB(NTADR_A(horz,vert));
	list[8+12*num]=(sco/10)%10+0x10;
	horz++;
	list[9+12*num]= MSB(NTADR_A(horz,vert));
	list[10+12*num]= LSB(NTADR_A(horz,vert));
	list[11+12*num]=(sco%10)+0x10;
}

void set_background(const int j)
{
	ppu_off();
	vram_adr(NAMETABLE_A);
	if(j==0)
	{
		vram_unrle(titre);
	}
	else
	{
		vram_unrle(playfield);
	}
	ppu_on_all();
}

// BOITE
void initBox(){
	//odd
	boite[0][0] = 1;
	boite[0][1] = 3;
	boite[0][2] = 5;
	//boite[0][3] = 7;
		
	//neutral
	boite[1][0] = 0;
	boite[1][1] = 0;
	boite[1][2] = 0;
	//boite[1][3] = 0;
		
	//even
	boite[2][0] = 2;
	boite[2][1] = 4;
	boite[2][2] = 6;
	//boite[2][3] = 8;
}


void setShifter(int desiredDirection){
	int pop = MAX_COL-1;
	//put_int(NTADR_A(2,6),pop);
	//si on est sur une vitesse neutre
	if (boite[shifterPosY][shifterPosX] == 0){
		
		//si on est sur le neutre tout à gauche
		if (shifterPosX == 0) {
			
			//alors toutes directions possibles sauf GAUCHE
			switch (desiredDirection){
				case DIR_LEFT:break;
				case DIR_UP:shifterPosY--;break;
				case DIR_RIGHT:shifterPosX++;break;
				case DIR_DOWN:shifterPosY++;break;
			}
			return;
		}
		//si on est sur le neutre tout à droite
		if (shifterPosX ==pop)
		{
		//alors toutes directions possibles sauf DROITE
			switch (desiredDirection)
			{
				case DIR_LEFT:shifterPosX--;break;
				case DIR_UP:shifterPosY--;break;
				case DIR_RIGHT:break;
				case DIR_DOWN:shifterPosY++;break;
			}
			return;
		}
	
		//sinon on est sur un des neutres centraux
		if(shifterPosX>0 && shifterPosX<pop)
		{
			//alors tous mouvements possibles
			switch (desiredDirection)
			{
				case DIR_LEFT:shifterPosX--;break;
				case DIR_UP:shifterPosY--;break;
				case DIR_RIGHT:shifterPosX++;break;
				case DIR_DOWN:shifterPosY++;break;
			}
		}
	}
	
	//si on est sur une vitesse
	else 
	{
			
		//si on est sur une vitesse paire : seul mouvement autorisé est UP
		if ((boite[shifterPosY][shifterPosX]%2 == 0) && desiredDirection == DIR_UP)
		{
			shifterPosY--;
		}	
		
		//si on est sur une vitesse impaire : seul mouvement autorisé est DOWN
		else if ((boite[shifterPosY][shifterPosX]%2 != 0) && desiredDirection == DIR_DOWN)
		{
			shifterPosY++;
		}
	}
 
}

void shfit_sprite_0()
{
	if(pad&PAD_LEFT ) 
	{
		dir_sprite = DIR_LEFT;
		state_sprite = 1;
	}
	if(pad&PAD_RIGHT)
	{
		dir_sprite = DIR_RIGHT;
		state_sprite = 1;
	}
	if(pad&PAD_UP ) 
	{
		dir_sprite = DIR_UP;
		state_sprite = 1;
	}
	if(pad&PAD_DOWN)
	{
		dir_sprite = DIR_DOWN;
		state_sprite = 1;
	}
}

void shfit_sprite_1()
{
	if(pad==0)state_sprite =0;
}

void shifter_machine()
{
	{
/*
	0 - Repos
	1 - déplacement
*/
	pad=pad_poll(0);
	if ( state_sprite == 0) shfit_sprite_0();
	if ( state_sprite == 1) shfit_sprite_1();
	
	}
}

void rev_machine()
{
	int rev_tmp;
	pad=pad_poll(0);
	rev_tmp=min_rev[boite[shifterPosY][shifterPosX]];
	if(pad&PAD_A && rev<MAX_REV && rev>rev_tmp)rev+=ACCEL;
	else rev-=DECCEL;
	if(rev<RALENTI)rev=RALENTI;
	if(rev>RED_ZONE)loose++;
	put_int(0,7,2,rev);
}

void speed_machine()
{
	int ratio_tmp,rev_tmp;
	ratio_tmp =ratio[boite[shifterPosY][shifterPosX]];
	rev_tmp=min_rev[boite[shifterPosY][shifterPosX]];
	if(ratio_tmp>0)
	{
		if((speed-20<rev / ratio_tmp||speed+20>rev / ratio_tmp)&&rev>rev_tmp)
		{
			speed = rev / ratio_tmp;
			sound_state=0;
		}
		else 
		{
			sound_state=1;
			loose++;
		}
	}
	else 
	{
		if(speed>0)speed--;
		sound_state=0;
	}
	if (speed> 220) win++;
	put_int(1,7,3,speed);
}

void shifter_move()
{
	/*
	if (dir_sprite == DIR_LEFT) shifter_x = shifter_x-40;
	if (dir_sprite == DIR_RIGHT) shifter_x = shifter_x+40;
	if (dir_sprite == DIR_UP) shifter_y = shifter_y-48;
	if (dir_sprite == DIR_DOWN) shifter_y = shifter_y+48;
	*/
	/*
	if(dir_sprite!=0)
	{
		put_int(0,2,2,dir_sprite);
		put_int(1,2,3,shifterPosX);
		put_int(2,2,4,shifterPosY);
	}
	*/
	/*
	if((shifterPosX*40)+76>shifter_x)shifter_x++;
	if((shifterPosX*40)+76<shifter_x)shifter_x--;	
	if((shifterPosY*48)+52>shifter_y)shifter_y++;	
	if((shifterPosY*48)+52<shifter_y)shifter_y--;
	*/	
	shifter_x=(shifterPosX*40)+76;
	shifter_y=(shifterPosY*48)+52;
	dir_sprite=0;
}

void launch_game()
{
	game_state =1;
	set_background(game_state);
	shifter_x = 116;
	shifter_y = 100;
	shifterPosX=1;
	shifterPosY=1;
	spr = 0;
	initBox();
	rev=RALENTI;
	speed=0;
	sound_state = 0;
	win = 0;
	loose = 0;
	
	put_str(NTADR_A(1,2),"REV");
	put_str(NTADR_A(1,3),"SPEED");
	put_str(NTADR_A(1,4),"DAMAG");
	/*
	while(1)
	{
		ppu_wait_frame();
		spr=oam_meta_spr(shifter_x,shifter_y,spr,metaShifter);
		shifter_machine();
		setShifter(dir_sprite);
		shifter_move();
	}
	*/
}
void son()
{
	if(sound_state==0)sfx_play((rev/625),0);
	if(sound_state==1)sfx_play(0,3);
}

void game_machine()
{
	if(win>30)game_state=2;
	if(loose>99)game_state=3;
	put_int(2,7,4,loose);
}


void main(void)
{
	
	//rendering is disabled at the startup, and palette is all black
	pal_spr(palette);
	pal_bg(palette);//set background palette from an array
	
	list[3*12] = NT_UPD_EOF;
	set_vram_update(list);

	vram_adr(NAMETABLE_A);//set VRAM address
	vram_unrle(titre);//unpack nametable into VRAM

	ppu_on_all();//enable rendering
	
	game_state=0;
	input_state=0;
	wait=0;


	while(1)
	{
		ppu_wait_frame();
		if(game_state==0)
		{
			wait++;
			spr=oam_meta_spr(shifter_x,shifter_y,spr,fuckShifter);
			if(wait==50) put_str_up(10,24,"PRESS START"); //put_str(NTADR_A(10,24),"PRESS START");
			if(wait==100)
			{
				//put_str(NTADR_A(10,24),"           ");
				put_str_up(10,24,"           ");
				wait=0;
			}
			pad=pad_poll(0);
			if(pad&PAD_START && input_state==0)
			{
				put_str_up(10,24,"           ");
				launch_game();
			}
			if(pad==0)input_state=0;
		}
		if(game_state==1)
		{
			spr=oam_meta_spr(shifter_x,shifter_y,spr,metaShifter);
			shifter_x++;
			shifter_machine();
			setShifter(dir_sprite);
			/*
			if(dir_sprite==DIR_LEFT)shifterPosX--;
			if(dir_sprite==DIR_UP)shifterPosY--;
			if(dir_sprite==DIR_RIGHT)shifterPosX++;
			if(dir_sprite==DIR_DOWN)shifterPosY++;
			*/
			shifter_move();
			rev_machine();
			speed_machine();
			son();
			game_machine();
		}
		if(game_state==2)
		{
			wait++;
			if(wait==50) put_str_up(10,24,"IOU HOUINE"); //put_str(NTADR_A(10,24),"PRESS START");
			if(wait==100)
			{
				//put_str(NTADR_A(10,24),"           ");
				put_str_up(10,24,"           ");
				wait=0;
			}
			pad=pad_poll(0);
			if(pad&PAD_START)
			{
				put_str_up(10,24,"           ");
				game_state=0;
				input_state=1;
				set_background(game_state);
			}
		}
		if(game_state==3)
		{
			wait++;
			if(wait==50) put_str_up(10,24,"IOU LOUZE"); //put_str(NTADR_A(10,24),"PRESS START");
			if(wait==100)
			{
				//put_str(NTADR_A(10,24),"           ");
				put_str_up(10,24,"           ");
				wait=0;
			}
			pad=pad_poll(0);
			if(pad&PAD_START)
			{
				put_str_up(10,24,"           ");
				game_state=0;
				input_state=1;
				set_background(game_state);
			}
		}
	}
}
