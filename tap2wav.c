#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>

struct WAB_CNK {
	unsigned char name[4];
	unsigned long size;
} WCNK;

struct WAB_FMT {
	unsigned short id;			// フォーマットID
	unsigned short ch_cnt;	// チャネル数
	unsigned long	 hz;			// サンプリング周波数
	unsigned long	 spd;			// 平均データ速度
	unsigned short blk_siz; // ブロックサイズ
	unsigned short bit_siz; // １サンプル当たりのビット数
	unsigned short headext;	// ?
} WFMT;

unsigned char TMP0[0x1000];
unsigned char TMP1[0x4000];

short tap2wav(const char *tap_name,const char *wav_name)
{
	unsigned long flng,freq;
	unsigned short siz,lng;
	unsigned short off,err;
	unsigned char	bit,dmy;
	FILE *des;
	FILE *src;
	struct stat stbuf;

	err=0;
	src=des=NULL;

	src=fopen(tap_name,"rb");
	des=fopen(wav_name,"wb");
	if(src==NULL || des==NULL){
		if(src!=NULL) fclose(src);
		if(des!=NULL) fclose(des);
		remove(wav_name);
		return 1;
	}

	//-----

	if(stat(tap_name, &stbuf) == -1){
		fprintf(stderr, "cant get file state : %s.\n", tap_name);
		exit(EXIT_FAILURE);
	}

	flng = stbuf.st_size;
	fread(&freq,1,4,src);

	//-----

	memcpy(WCNK.name,"RIFF",4);
	// ファイル全体のサイズ−８
	WCNK.size=(flng-4l)*8l;
	WCNK.size+=46l-8l;
	fwrite(&WCNK,1,8,des);

	fwrite("WAVE",1,4,des);

	//-----

	memcpy(WCNK.name,"fmt ",4);
	WCNK.size=sizeof(struct WAB_FMT);
	fwrite(&WCNK,1,8,des);

	WFMT.id=1;
	WFMT.ch_cnt=1;
	WFMT.hz =freq;
	WFMT.spd=freq;
	WFMT.blk_siz=1;
	WFMT.bit_siz=8;
	WFMT.headext=0;
	fwrite(&WFMT,1,sizeof(struct WAB_FMT),des);

	//-----

	memcpy(WCNK.name,"data",4);
	WCNK.size=(flng-4l)*8l; // dataのサイズは必ず偶数
	fwrite(&WCNK,1,8,des);

	while(!err && flng){
		if(flng>0x800l){ lng=0x800; flng-=0x800l; }
		else{ lng=(short)flng; flng=0l; }

		fread(TMP0,1,lng,src); if(ferror(src)){ err=1; break; }

		siz=0; off=0; bit=0x80;
		while(off<lng){
			TMP1[siz++]=((TMP0[off]&bit)?0xff:0x00);
			bit>>=1; if(!bit){ bit=0x80; off++; }
		}

		fwrite(TMP1,1,siz,des); if(ferror(des)){ err=2; break; }
	}

	//-----

	fclose(src);
	fclose(des);
	if(err) remove(wav_name);

	return err;
}

int main(int argc,char *argv[])
{
	const char *src;
	const char *des;
	short i,err;

	err=0; src=des=NULL;
	for(i=1; i<argc && !err; i++){
		if(argv[i][0]=='/' || argv[i][0]=='-'){
			err=1;
		} else {
			if(src==NULL) src=argv[i];
			else if(des==NULL) des=argv[i];
			else err=1;
		}
	}
	if(src==NULL || des==NULL || err){
		printf("tap2wav (Ver.0.2)\n");
		printf("	tapからwavへコンバートします\n");
		printf("	tap2wav [元ファイル.tap] [先ファイル.wav]\n");
		exit(1);
	}

	if(tap2wav(src,des)) printf("Error!\n");
	else								 printf("Ok.\n");
}
