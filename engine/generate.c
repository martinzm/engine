
#include "bitmap.h"
#include "evaluate.h"
#include "hash.h"
#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include "search.h"
#include "globals.h"

// rook moves
void generate_rook(BITVAR norm[])
{
int f,m,r,x;
	
BITVAR q1;
	
	for(f=0;f<8;f++) {
		for(m=0;m<8;m++) {
			q1 = EMPTYBITMAP;
			for(r=0;r<m;r++) q1=SetNorm(f*8+r, q1);			
			for(r=m+1;r<8;r++) q1=SetNorm(f*8+r, q1);
			for(x=0;x<f;x++) q1=SetNorm(x*8+m, q1);
			for(x=f+1;x<8;x++) q1=SetNorm(x*8+m, q1);
			norm[f*8+m]=q1;
//			printmask(q1,"rooks");
		}
	}
}

// bishop moves
void generate_bishop(BITVAR norm[])
{
int f,n, z1,z2;
	
BITVAR q1;
	
	for(f=0;f<8;f++) {
		for(n=0;n<8;n++) {
			q1 = EMPTYBITMAP;
			z1=n-1;
			z2=f-1;
			while((z1>=0 && z2>=0)) {
				q1=SetNorm(z2*8+z1, q1);
				z1--;
				z2--;
			}
			z1=n+1;
			z2=f-1;
			while((z1<8 && z2>=0)) {
				q1=SetNorm(z2*8+z1, q1);
				z1++;
				z2--;
			}
			z1=n-1;
			z2=f+1;
			while((z1>=0 && z2<8)) {
				q1=SetNorm(z2*8+z1, q1);
				z1--;
				z2++;
			}
			z1=n+1;
			z2=f+1;
			while((z1<8 && z2<8)) {
				q1=SetNorm(z2*8+z1, q1);
				z1++;
				z2++;
			}
			norm[f*8+n]=q1;
//			printmask(q1);
		}
	}
}

//knight moves
void generate_knight(BITVAR norm[])
{
int f,n,r,x,y;
int moves[]= { -2,-1, -1, -2, 1, -2, 2, -1, 2, 1, 1, 2, -1, 2, -2,1 };	
	
BITVAR q1;
	
	for(f=0;f<8;f++) {
		for(n=0;n<8;n++) {
			q1= EMPTYBITMAP;
			
			for(r=0;r<8;r++) {
				x=n+moves[r*2];
				y=f+moves[r*2+1];
				if((x>=0 && x<8) && (y>=0 && y<8)) {
						q1=SetNorm(y*8+x, q1);
				}
			}
			norm[f*8+n]=q1;
//			printmask(q1);
		}
	}
}

// king moves
void generate_king(BITVAR norm[])
{
int f,n,r,x,y;
int moves[]= { -1,-1, -1, 0, -1, 1, 0, -1, 0, 1, 1, -1, 1, 0, 1,1 };

BITVAR q1;
	
	for(f=0;f<8;f++) {
		for(n=0;n<8;n++) {
			q1 = EMPTYBITMAP;
			for(r=0;r<8;r++) {
				x=n+moves[r*2];
				y=f+moves[r*2+1];
				if((x>=0 && x<8) && (y>=0 && y<8)) {
						q1=SetNorm(y*8+x, q1);
				}
			}
			norm[f*8+n]=q1;
//			printmask(q1);
		}
	}
}

// white pawn moves
void generate_w_pawn_moves(BITVAR norm[])
{
int f,n;
	
BITVAR q1;
	
	for(n=0;n<8;n++) {
			norm[n]=EMPTYBITMAP;
			norm[56+n]=EMPTYBITMAP;
	}
	for(n=0;n<8;n++) {
			q1 = SetNorm(16+n,EMPTYBITMAP)|SetNorm(24+n,EMPTYBITMAP);
			norm[8+n]=q1;
	}

	for(f=2;f<7;f++) {
		for(n=0;n<8;n++) {
				q1=SetNorm((f+1)*8+n, EMPTYBITMAP);
				norm[f*8+n]=q1;
		}
	}
}

// white pawn attacks
void generate_w_pawn_attack(BITVAR norm[])
{
int f,n,r,x,y;

int moves[]= { -1,1, 1,1 };	

	
BITVAR q1;
	
	q1 = EMPTYBITMAP;

	for(n=0;n<63;n++) {
			norm[n]=EMPTYBITMAP;
	}

	for(f=0;f<7;f++) {
		for(n=0;n<8;n++) {
			q1 = EMPTYBITMAP;
			for(r=0;r<2;r++) {
				x=n+moves[r*2];
				y=f+moves[r*2+1];
				if((x>=0 && x<8) && (y>=0 && y<8)) {
						q1=SetNorm(y*8+x, q1);
				}
			}
			norm[f*8+n]=q1;
		}
	}

}

// black pawn moves
void generate_b_pawn_moves(BITVAR norm[])
{
int f,n;
	
BITVAR q1;
	
	for(n=0;n<63;n++) {
			norm[n]=EMPTYBITMAP;
//			norm[56+n]=EMPTYBITMAP;
	}
	for(n=0;n<8;n++) {
			q1 = SetNorm(40+n,EMPTYBITMAP)|SetNorm(32+n,EMPTYBITMAP);
			norm[48+n]=q1;		
	}

	for(f=1;f<6;f++) {
		for(n=0;n<8;n++) {
				q1=SetNorm((f-1)*8+n, EMPTYBITMAP);
				norm[f*8+n]=q1;
		}
	}
}

// black pawn attacks
void generate_b_pawn_attack(BITVAR norm[])
{
int f,n,r,x,y;

int moves[]= { -1,-1, 1,-1 };	

	
BITVAR q1;
	
	q1 = EMPTYBITMAP;

	for(n=0;n<63;n++) {
			norm[n]=EMPTYBITMAP;		
	}

	for(f=1;f<8;f++) {
		for(n=0;n<8;n++) {
			q1 = EMPTYBITMAP;
			for(r=0;r<2;r++) {
				x=n+moves[r*2];
				y=f+moves[r*2+1];
				if((x>=0 && x<8) && (y>=0 && y<8)) {
						q1=SetNorm(y*8+x, q1);
				}
			}
			norm[f*8+n]=q1;
		}
	}
}

// 
void generate_topos(int *ToPos)
{
int n,r;
int f;
		n=0;
		for(f=0;f<=16;f++) {
			r=1<<f;
			while(n<r) {
				ToPos[n++]=(int)(f-1);
			}
		}
}

// square to square bitmap including from & to, rays_int doesnt have from & to squares
void generate_rays(BITVAR rays[64][64], BITVAR rays_int[64][64])
{
BITVAR t, t2;
int f,n,x,y;
	
		for(f=0;f<64;f++) 
			for(n=0;n<64;n++) rays[f][n]=EMPTYBITMAP;
				
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				
// generate rady
				t=EMPTYBITMAP;
				x=n;
				y=f;
				while((x>=0) && (x<8) && (y>=0)&&(y<8)) {
					t=SetNorm(y*8+x,t);
					rays[f*8+n][y*8+x]=t;
					rays[y*8+x][f*8+n]=t;
// odstranime zdroj, destinaci a ulozime do rays_int
					t2= t & (~normmark[f*8+n]) & (~normmark[y*8+x]);
					rays_int[f*8+n][y*8+x]=t2;
					rays_int[y*8+x][f*8+n]=t2;
					x++;
				}
								
// generate sloupce
				t=EMPTYBITMAP;
				x=n;
				y=f;
				while((x>=0) && (x<8) && (y>=0)&&(y<8)) {
					t=SetNorm(y*8+x,t);
					rays[f*8+n][y*8+x]=t;
					rays[y*8+x][f*8+n]=t;
// odstranime zdroj, destinaci a ulozime do rays_int
					t2= t & (~normmark[f*8+n]) & (~normmark[y*8+x]);
					rays_int[f*8+n][y*8+x]=t2;
					rays_int[y*8+x][f*8+n]=t2;
					y++;
				}
												
// generate uhlopricka leva
				t=EMPTYBITMAP;
				x=n;
				y=f;
				while((x>=0) && (x<8) && (y>=0)&&(y<8)) {
					t=SetNorm(y*8+x,t);
					rays[f*8+n][y*8+x]=t;
					rays[y*8+x][f*8+n]=t;
// odstranime zdroj, destinaci a ulozime do rays_int
					t2= t & (~normmark[f*8+n]) & (~normmark[y*8+x]);
					rays_int[f*8+n][y*8+x]=t2;
					rays_int[y*8+x][f*8+n]=t2;
					y++;
					x--;
				}

// generate uhlopricka prava
				t=EMPTYBITMAP;
				x=n;
				y=f;
				while((x>=0) && (x<8) && (y>=0)&&(y<8)) {
					t=SetNorm(y*8+x,t);
					rays[f*8+n][y*8+x]=t;
					rays[y*8+x][f*8+n]=t;
// odstranime zdroj, destinaci a ulozime do rays_int
					t2= t & (~normmark[f*8+n]) & (~normmark[y*8+x]);
					rays_int[f*8+n][y*8+x]=t2;
					rays_int[y*8+x][f*8+n]=t2;
					y++;
					x++;
				}
			}
		}
}

void gen_rays_dir2(BITVAR rd[64][64], int x,int y,int dx,int dy){
BITVAR t;
int xx,yy;

// go to end of ray
	t=EMPTYBITMAP;
	xx=x+dx;
	yy=y+dy;
	while((xx>=0) && (xx<8) && (yy>=0)&&(yy<8)) {
		t|=normmark[yy*8+xx];
		yy+=dy;
		xx+=dx;
	}

// trace back
	xx-=dx;
	yy-=dy;
	while((xx!=x)||(yy!=y)) {
		rd[y*8+x][yy*8+xx]=t;
		yy-=dy;
		xx-=dx;
	}
}

// generate rays from start square in direction of second square until border
void generate_rays_dir(BITVAR rays_dir[64][64])
{
BITVAR t;
int x,y,dir, x2, y2;
int hv[] = { 0,1, 1,1, 1,0, 1,-1, 0,-1, -1,-1, -1,0, -1,1 };
char buf[256];


/*
 * dir: 0=up, 1=upright, 2=right, 3=downright, 4=down, 5=downleft, 6=left, 7=upleft
 */
	for(y=0;y<64;y++) 
		for(x=0;x<64;x++) rays_dir[y][x]=EMPTYBITMAP;

	for(y=0;y<8;y++)
		for(x=0;x<8;x++) {
			for(dir=0;dir<8;dir++) {
				gen_rays_dir2(rays_dir,x,y,hv[dir*2],hv[dir*2+1]);
			}
		}
/*
	for(y=0;y<8;y++) 
		for(x=0;x<8;x++)
			for(y2=0;y2<8;y2++)
				for(x2=0;x2<8;x2++) {
					t=rays_dir[y*8+x][y2*8+x2];
					if(t!=0) {
						sprintf(buf,"Direction X:%d, Y:%d ==> X2:%d, Y2:%d",x,y,x2,y2);
						printmask(t, buf);
					}
				}
*/
}

BITVAR gen_dir(int sx, int sy, int h, int v)
{
BITVAR ret=0;
	while(
		((sx+h)<=7)&&((sx+h)>=0)
		&&((sy+v)<=7)&&((sy+v)>=0)) {
		sx+=h;
		sy+=v;
		ret|=normmark[sy*8+sx];
	}
	return ret;
}

// directions from square
void generate_directions(BITVAR directions[64][8])
{
int x,y,sq,dir;
int hv[] = { 0,1, 1,1, 1,0, 1,-1, 0,-1, -1,-1, -1,0, -1,1 };

/*
 * dir: 0=up, 1=upright, 2=right, 3=downright, 4=down, 5=downleft, 6=left, 7=upleft
 */
	for(y=0;y<8;y++)
		for(x=0;x<8;x++) {
			sq=y*8+x;
			for(dir=0;dir<8;dir++) {
				directions[sq][dir]=gen_dir(x,y,hv[dir*2],hv[dir*2+1]);
			}
		}
}


void generate_w_passed_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=1;y<7;y++) {
		    m=attack.rays[(y+1)*8+x][A7+x];
		    if(x>0) m|=attack.rays[(y+1)*8+x-1][A7+x-1];
		    if(x<7) m|=attack.rays[(y+1)*8+x+1][A7+x+1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "MaskW");
}

void generate_w_back_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=1;y<7;y++) {
			m=0;
		    if(x>0) m|=attack.rays[(y-1)*8+x-1][A2+x-1];
		    if(x<7) m|=attack.rays[(y-1)*8+x+1][A2+x+1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "MaskW");
}

void generate_b_passed_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=1;y<7;y++) {
		    m=attack.rays[(y-1)*8+x][A2+x];
		    if(x>0) m|=attack.rays[(y-1)*8+x-1][A2+x-1];
		    if(x<7) m|=attack.rays[(y-1)*8+x+1][A2+x+1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "MaskB");
}

void generate_b_back_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=1;y<7;y++) {
		    m=0;
		    if(x>0) m|=attack.rays[(y+1)*8+x-1][A7+x-1];
		    if(x<7) m|=attack.rays[(y+1)*8+x+1][A7+x+1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "MaskB");
}

void generate_file_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
//	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=0;y<8;y++) {
	    	m= attack.rays[A8+x][x+A1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "File");
}

void generate_rank_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
//	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(y=0;y<8;y++) {
		for(x=0;x<8;x++) {
			m= attack.rays[y*8+A1][y*8+H1];
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "Rank");
}

void generate_iso_w_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
		m=EMPTYBITMAP;
		if(x>0) m|=attack.rays[A8+x-1][x-1];
		if(x<7) m|=attack.rays[A8+x+1][x+1];
	    for(y=1;y<7;y++) {
		    map[y*8+x]=m;
	    }
	}
//	for(x=0;x<64;x++) printmask(map[x], "iSoMaskW");
}

void generate_color_map(int map[64])
{
int f;
    for(f=0;f<64;f+=2) map[f]=BLACK;
    for(f=1;f<64;f+=2) map[f]=WHITE;
}

// chebyshev distance
void generate_distance(int map[64][64])
{
int x1,x2,y1,y2,x,y,r;

	for(y1=0;y1<8;y1++)
		for(x1=0;x1<8;x1++) 
			for(y2=0;y2<8;y2++) 
				for(x2=0;x2<8;x2++) {
					x=(x2>x1 ? x2-x1 : x1-x2);
					y=(y2>y1 ? y2-y1 : y1-y2);
					r=(x>y ? x : y);
					map[y1*8+x1][y2*8+x2]=r;
				}
//	for(y1=0;y1<8;y1++)
//		for(x1=0;x1<8;x1++) {
//			printf("\n");
//			for(y2=7;y2>=0;y2--) {
//				printf("+-+-+-+-+-+-+-+-+\n");
//				for(x2=0;x2<8;x2++) {
//					printf("|%d",map[y1*8+x1][y2*8+x2]);
//				}
//				printf("|\n");
//			}
//			printf("+-+-+-+-+-+-+-+-+\n");
//			printf("\n");
//		}
}

// manhattan distance
void generate_distance2(int map[64][64])
{
int x1,x2,y1,y2,x,y,r;

	for(y1=0;y1<8;y1++)
		for(x1=0;x1<8;x1++) 
			for(y2=0;y2<8;y2++) 
				for(x2=0;x2<8;x2++) {
					x=(x2>x1 ? x2-x1 : x1-x2);
					y=(y2>y1 ? y2-y1 : y1-y2);
					r=x+y;
					map[y1*8+x1][y2*8+x2]=r;
				}
//	for(y1=0;y1<8;y1++)
//		for(x1=0;x1<8;x1++) {
//			printf("\n");
//			for(y2=7;y2>=0;y2--) {
//				printf("+-+-+-+-+-+-+-+-+\n");
//				for(x2=0;x2<8;x2++) {
//					printf("|%d",map[y1*8+x1][y2*8+x2]);
//				}
//				printf("|\n");
//			}
//			printf("+-+-+-+-+-+-+-+-+\n");
//			printf("\n");
//		}
}





void generate_uphalf(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y,z;
	
	b=EMPTYBITMAP;
//	for(x=A1;x<=A8;x++) map[x]=b;
	for(x=A8;x<=H8;x++) map[x]=b;
	for(y=6;y>=0;y--) {
		b=EMPTYBITMAP;
		for(z=7;z>y;z--) {
			b|=att.rank[z*8];
		}
		for(x=0;x<8;x++) map[y*8+x]=b;
	}
//	for(z=0;z<64;z++) printmask(map[z], "UP\n");
}

void generate_downhalf(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y,z;
	
	b=EMPTYBITMAP;
	for(x=A1;x<=A8;x++) map[x]=b;
	for(y=1;y<8;y++) {
		b=EMPTYBITMAP;
		for(z=0;z<y;z++) {
			b|=att.rank[z*8];
		}
		for(x=0;x<8;x++) map[y*8+x]=b;
	}
//	for(z=0;z<64;z++) printmask(map[z], "Down\n");
}

void generate_lefthalf(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y,z;
	
	b=EMPTYBITMAP;
	for(y=0;y<8;y++) map[y*8+A1]=b;
	for(x=1;x<8;x++) {
		b=EMPTYBITMAP;
		for(z=0;z<x;z++) {
			b|=att.file[z];
		}
		for(y=0;y<8;y++) map[y*8+x]=b;
	}
//	for(z=0;z<64;z++) printmask(map[z], "Left\n");
}

void generate_righthalf(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y,z;
	
	b=EMPTYBITMAP;
	for(y=0;y<8;y++) map[y*8+H1]=b;
	for(x=0;x<7;x++) {
		b=EMPTYBITMAP;
		for(z=x+1;z<8;z++) {
			b|=att.file[z];
		}
		for(y=0;y<8;y++) map[y*8+x]=b;
	}
//	for(z=0;z<64;z++) printmask(map[z], "Right\n");
}

// generata pawn 
void generate_pawn_surr(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y;
	
	for(x=0;x<8;x++) 
		for(y=0;y<8;y++) {
			b=EMPTYBITMAP;
			if(y>0) {
				b|=(att.file[x]&att.rank[(y-1)*8]);
				if(x>0) b|=(att.file[x-1]&att.rank[(y-1)*8]);
				if(x<7) b|=(att.file[x+1]&att.rank[(y-1)*8]);
			}
			if(y<7) {
				b|=(att.file[x]&att.rank[(y+1)*8]);
				if(x>0) b|=(att.file[x-1]&att.rank[(y+1)*8]);
				if(x<7) b|=(att.file[x+1]&att.rank[(y+1)*8]);
			}
			if(x>0) b|=(att.file[x-1]&att.rank[y*8]);
			if(x<7) b|=(att.file[x+1]&att.rank[y*8]);	
			map[y*8+x]=b;
//			printmask(map[y*8+x], "\nPawn surround");
		}
//	for(z=0;z<64;z++) 
//			printmask(map[z], "\nPawn surround");
}

void generate_attack_r45R(BITVAR map[64][256], int skip)
{
int xu,yu,xl,yl,f,n, tskip;
unsigned int u,z,zt,l;

		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound
					u=1U<<ind45R[f*8+n];
					zt=z & ~u;
					tskip=skip;
					while((xu<7) && (yu<7) && (u<128)) {
						if(zt & u) {
						  if(tskip<=0) break; else tskip--;
						}
						u<<=1;
						yu++;
						xu++;
					}
// lower bound
					xl=n;
					yl=f;
					l=1U<<ind45R[f*8+n];
					tskip=skip;
					zt=z & ~l;
					while((xl>=0)  && (yl>=0) && (l>1)) {
						if(zt & l) {
						  if(tskip<=0) break; else tskip--;
						}
						l>>=1;
						yl--;
						xl--;
					}
// find 	proper ray 
					map[f*8+n][z]=attack.rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind45R[f*8+n]);
//					printmask(map[f*8+n][z]);
				}
			}
		}
}

void generate_attack_r45L(BITVAR map[64][256], int skip)
{
int xu,yu,xl,yl,f,n, tskip;
unsigned int u,z,zt,l;

		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound
					u=1U<<ind45L[f*8+n];
					zt=z & ~u;
					tskip=skip;
					while((xu>0)  && (yu<7) && (u<128)) {
						if(zt & u) {
						  if(tskip<=0) break; else tskip--;
						}
						u<<=1;
						yu++;
						xu--;
					}
// lower bound
					xl=n;
					yl=f;
					l=1U<<ind45L[f*8+n];;
					zt=z & ~l;
					tskip=skip;
					while((xl<7) && (yl>0)  && (l>1)) {
						if(zt & l) {
						  if(tskip<=0) break; else tskip--;
						}
						l>>=1;
						yl--;
						xl++;
					}
// find 	proper ray 
					map[f*8+n][z]=attack.rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind45L[f*8+n]);
//					printmask(map[f*8+n][z]);
				}
			}
		}
}

void generate_attack_r90R(BITVAR map[64][256], int skip)
{
int xu,yu,xl,yl,f,n, tskip;
unsigned int u,z,zt,l;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound
					u=1U<<ind90[f*8+n];
					zt=z & ~u;
					tskip=skip;
					while((yu<7) && (u<128)) {
						if(zt & u) {
						  if(tskip<=0) break; else tskip--;
						}
						u<<=1;
						yu++;
					}
// lower bound
					xl=n;
					yl=f;

					l=1U<<ind90[f*8+n];
					zt=z & ~l;
					tskip=skip;
					while((yl>=0) && (l>1)) {
						if(zt & l) {
						  if(tskip<=0) break; else tskip--;
						}
						l>>=1;
						yl--;
					}
// find 	proper ray 
					map[f*8+n][z]=attack.rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind90[f*8+n]);
//					printmask(map[f*8+n][z],"!!!");
				}
			}
		}
}

void generate_attack_norm(BITVAR map[64][256], int skip)
{
int xu,yu,xl,yl,f,n, tskip;
unsigned int u,z,zt,l;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound
					u=1U<<indnorm[f*8+n];;
					zt=z & ~u;
					tskip=skip;
					while((xu<7) && (u<128)) {
						if(zt & u) {
						  if(tskip<=0) break; else tskip--;
						}
						u<<=1;
						xu++;
					}
// lower bound
					xl=n;
					yl=f;
					l=1U<<indnorm[f*8+n];;
					zt=z & ~l;
					tskip=skip;
					while((xl>=0)  && (l>1)) {
						if(zt & l) {
						  if(tskip<=0) break; else tskip--;
						}
						l>>=1;
						xl--;
					}
// find 	proper ray 
					map[f*8+n][z]=attack.rays[yl*8+xl][yu*8+xu];
// clear the position from bitmap
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, indnorm[f*8+n]);
//					printmask(map[f*8+n][z], "mmmmm");
				}
			}
		}
}

// generate masks that allows for position to check possible attackers
void generate_ep_mask(BITVAR norm[])
{
int f;
		for(f=0;f<64;f++) norm[f]=EMPTYBITMAP;
		for(f=1;f<7;f++) {
			norm[A5+f]=SetNorm(A5+f-1,EMPTYBITMAP) | SetNorm(A5+f+1,EMPTYBITMAP);
			norm[A4+f]=SetNorm(A4+f-1,EMPTYBITMAP) | SetNorm(A4+f+1,EMPTYBITMAP);
		}
		norm[A5]=SetNorm(B5,EMPTYBITMAP);
		norm[H5]=SetNorm(G5,EMPTYBITMAP);

		norm[A4]=SetNorm(B4,EMPTYBITMAP);
		norm[H4]=SetNorm(G4,EMPTYBITMAP);
}

void empty_board(board *b)
{
int f;
		for(f=A1;f<=H8;f++) b->pieces[f]=ER_PIECE;
		b->maps[PAWN]=EMPTYBITMAP;
		b->maps[KNIGHT]=EMPTYBITMAP;
		b->maps[BISHOP]=EMPTYBITMAP;
		b->maps[ROOK]=EMPTYBITMAP;
		b->maps[QUEEN]=EMPTYBITMAP;
		b->maps[KING]=EMPTYBITMAP;

		b->norm=EMPTYBITMAP;
		b->r45L=EMPTYBITMAP;
		b->r45R=EMPTYBITMAP;
		b->r90R=EMPTYBITMAP;
	
		b->colormaps[WHITE]=EMPTYBITMAP;
		b->colormaps[BLACK]=EMPTYBITMAP;
	
		b->ep=-1;
		b->side=WHITE;	// white

#if 0
		b->material[WHITE][PAWN]=0;
		b->material[WHITE][QUEEN]=0;
		b->material[WHITE][BISHOP]=0;
		b->material[WHITE][ROOK]=0;
		b->material[WHITE][KNIGHT]=0;
		b->material[WHITE][KING]=0;

		b->material[BLACK][PAWN]=0;
		b->material[BLACK][QUEEN]=0;
		b->material[BLACK][BISHOP]=0;
		b->material[BLACK][ROOK]=0;
		b->material[BLACK][KNIGHT]=0;
		b->material[BLACK][KING]=0;
#endif

		b->psq_b=0;
		b->psq_e=0;

		b->castle[WHITE]=0;
		b->castle[BLACK]=0;
		b->move=0;
		b->rule50move=0;
		b->key=0;
		b->gamestage=OPENING;
		
		b->king[WHITE]=-1;
		b->king[BLACK]=-1;
}

int collect_material_from_board(board const *b, int *pw, int *pb, int *nw, int *nb, int *bwl, int *bwd,
					  int *bbl, int *bbd, int *rw, int *rb, int *qw, int *qb) {
	*pw=BitCount(b->maps[PAWN]&b->colormaps[WHITE]);
	*nw=BitCount(b->maps[KNIGHT]&b->colormaps[WHITE]);
	*bwl=BitCount(b->maps[BISHOP]&WHITEBITMAP&b->colormaps[WHITE]);
	*bwd=BitCount(b->maps[BISHOP]&BLACKBITMAP&b->colormaps[WHITE]);
	*rw=BitCount(b->maps[ROOK]&b->colormaps[WHITE]);
	*qw=BitCount(b->maps[QUEEN]&b->colormaps[WHITE]);

	*pb=BitCount(b->maps[PAWN]&b->colormaps[BLACK]);
	*nb=BitCount(b->maps[KNIGHT]&b->colormaps[BLACK]);
	*bbl=BitCount(b->maps[BISHOP]&WHITEBITMAP&b->colormaps[BLACK]);
	*bbd=BitCount(b->maps[BISHOP]&BLACKBITMAP&b->colormaps[BLACK]);
	*rb=BitCount(b->maps[ROOK]&b->colormaps[BLACK]);
	*qb=BitCount(b->maps[QUEEN]&b->colormaps[BLACK]);

return 1;
}

void setup_FEN_board(board *b, char * fen)
{
int pos, val, x,y, rule50;
int bwl, bwd, bbl, bbd;
int pw, pb, nw, nb, rw, rb, qw, qb;
personality *p;

/* 
   FEN has 6 fields composed of ASCII chars separated by one space
   field 1: piece placement - white uppercase, Black lowercase
		board 8th row to 1st, empty squares - count of empty squares
		"/" separates data on adjacent ranks
		
   field 2: active color - w white, b black
   field 3: castling availability - "-" no possibility, "K" for kingside,
		"Q" for queenside, upper for white, lower for black
   field 4: E.P. "-" no, letter for file, digit for rank (3 black active,
		 6 white active)
		  En passant target square in algebraic notation. 
		  If there's no en passant target square, this is "-". 
		  If a pawn has just made a two-square move, this is the position "behind" the pawn. 
		  This is recorded regardless of whether there is a pawn in position to make an en passant capture.
   field 5: halfmove clock - count of plies since pawn or capture, reset to 0 after capture or pawn move
   field 6: fullmove - number of moves played, starts at 1
*/


		empty_board(b);
		p=b->pers;
		
//		LOGGER_0("pers %d\n", b->pers->use_hash);
//		LOGGER_0("pers %d\n", p->rook_on_semiopen[0]);
		x=0;
		y=7;
		pos=63;

		while(fen!=NULL) {
			if(*fen == ' ') {
				fen++;
				break;
			} else if(isdigit(*fen)) {
				val=*fen-'0';
				x+=val;
				fen++;
			} 
			else if(*fen=='/') {
				fen++;
				x=0;
				y--;
			}
			else {
				switch (*fen) {
					case 'k' : SetAll(y*8+x, BLACK, KING, b);
								  b->king[BLACK]=(int8_t)(y*8+x);
								  b->psq_b-=p->piecetosquare[0][BLACK][KING][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][KING][y*8+x];
//								  LOGGER_0("BK psq b:e %d:%d, x:y:t %d:%d:%d\n", b->psq_b, b->psq_e,x,y,y*+x);
								break;
					case 'n' : SetAll(y*8+x, BLACK, KNIGHT, b);
								  b->psq_b-=p->piecetosquare[0][BLACK][KNIGHT][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][KNIGHT][y*8+x];
								break;
					case 'q' : SetAll(y*8+x, BLACK, QUEEN, b);
								  b->psq_b-=p->piecetosquare[0][BLACK][QUEEN][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][QUEEN][y*8+x];
								break;
					case 'r' : SetAll(y*8+x, BLACK, ROOK, b);
								  b->psq_b-=p->piecetosquare[0][BLACK][ROOK][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][ROOK][y*8+x];
								break;
					case 'p' : SetAll(y*8+x, BLACK, PAWN, b);
								  b->psq_b-=p->piecetosquare[0][BLACK][PAWN][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][PAWN][y*8+x];
//								  LOGGER_0("BP %d %d\n",p->piecetosquare[0][BLACK][PAWN][y*8+x], y*8+x);
								break;
					case 'b' : SetAll(y*8+x, BLACK, BISHOP, b);
								  b->psq_b-=p->piecetosquare[0][BLACK][BISHOP][y*8+x];
								  b->psq_e-=p->piecetosquare[1][BLACK][BISHOP][y*8+x];
								break;
					case 'K' : SetAll(y*8+x, WHITE, KING, b);
								  b->king[WHITE]=(int8_t)(y*8+x);
								  b->psq_b+=p->piecetosquare[0][WHITE][KING][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][KING][y*8+x];
//								  LOGGER_0("WK psq b:e %d:%d, x:y:t %d:%d:%d\n", b->psq_b, b->psq_e,x,y,y*+x);
								break;
					case 'N' : SetAll(y*8+x, WHITE, KNIGHT, b);
								  b->psq_b+=p->piecetosquare[0][WHITE][KNIGHT][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][KNIGHT][y*8+x];
								break;
					case 'Q' : SetAll(y*8+x, WHITE, QUEEN, b);
								  b->psq_b+=p->piecetosquare[0][WHITE][QUEEN][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][QUEEN][y*8+x];
								break;
					case 'R' : SetAll(y*8+x, WHITE, ROOK, b);
								  b->psq_b+=p->piecetosquare[0][WHITE][ROOK][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][ROOK][y*8+x];
								break;
					case 'P' : SetAll(y*8+x, WHITE, PAWN, b);
								  b->psq_b+=p->piecetosquare[0][WHITE][PAWN][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][PAWN][y*8+x];
//								  LOGGER_0("psq b:e %d:%d\n", b->psq_b, b->psq_e);
								break;
					case 'B' : SetAll(y*8+x, WHITE, BISHOP, b);
								  b->psq_b+=p->piecetosquare[0][WHITE][BISHOP][y*8+x];
								  b->psq_e+=p->piecetosquare[1][WHITE][BISHOP][y*8+x];
								break;
					default : printf("ERROR!\n");
				}
				fen++;
				x++;
			}
		}
// material counts
#if 0
		b->material[WHITE][PAWN]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[PAWN])));
		b->material[WHITE][KNIGHT]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[KNIGHT])));
		b->material[WHITE][BISHOP]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[BISHOP])));
		b->material[WHITE][ROOK]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[ROOK])));
		b->material[WHITE][QUEEN]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[QUEEN])));
		b->material[WHITE][KING]= (uint8_t)(BitCount((b->colormaps[WHITE])&(b->maps[KING])));

		b->material[BLACK][PAWN]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[PAWN])));
		b->material[BLACK][KNIGHT]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[KNIGHT])));
		b->material[BLACK][BISHOP]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[BISHOP])));
		b->material[BLACK][ROOK]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[ROOK])));
		b->material[BLACK][QUEEN]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[QUEEN])));
		b->material[BLACK][KING]= (uint8_t)(BitCount((b->colormaps[BLACK])&(b->maps[KING])));
#endif 

		while(fen!=NULL) {
			if(*fen=='w') b->side=WHITE;
			else if(*fen=='b')  b->side=BLACK;
			else if(*fen==' ')  {
				fen++;
				break;
			} else {
				printf("Error\n");
			}
			fen++;
		}
		while(fen!=NULL) {
			switch (*fen) {
			case 'q' :
				if((b->pieces[E8]!=(KING|BLACKPIECE)) || (b->pieces[A8]!=(ROOK|BLACKPIECE))) {
					LOGGER_1("ERR: Castling: Black King to Queen side problem!\n");
					break;
				}
				b->castle[BLACK]=(int8_t)(b->castle[BLACK]+QUEENSIDE);
				break;
			case 'Q' :
				if((b->pieces[E1]!=(KING)) || (b->pieces[A1]!=(ROOK))) {
					LOGGER_1("ERR: Castling: White King to Queen side problem!\n");
					break;
				}
				b->castle[WHITE]=(int8_t)(b->castle[WHITE]+QUEENSIDE);
				break;
			case 'k' :
				if((b->pieces[E8]!=(KING|BLACKPIECE)) || (b->pieces[H8]!=(ROOK|BLACKPIECE))) {
					LOGGER_1("ERR: Castling: Black King to King side problem!\n");
					break;
				}
				b->castle[BLACK]=(int8_t)(b->castle[BLACK]+KINGSIDE);
				break;
			case 'K' :
				if((b->pieces[E1]!=(KING)) || (b->pieces[H1]!=(ROOK))) {
					LOGGER_1("ERR: Castling: White King to King side problem!\n");
					break;
				}
				b->castle[WHITE]=(int8_t)(b->castle[WHITE]+KINGSIDE);
				break;
			case '-' :
				break;
			case ' ' :
				break;
			default:
				printf("Castle input Error\n");
			}				
			fen++;
			if(*fen==' ') {
				fen++;
				break;
			}
		}
		if(*fen=='-') {
			fen++;
		} 
		else {

/*
 * EP field in FEN represents destination of PAWN performing ep capture
 * in b->ep field we have real position of PAWN captured by ep capture
 * for example white pawn at H moved 2 ranks to H4
 * so in FEN EP field is H3
 * while in b->ep there is H4
 */
 
			pos=(toupper((*fen++)) - 'A') ;
			pos+=(((*fen++)-'1')*8);
			if(b->side==WHITE) pos-=8; else pos+=8;
			b->ep=(int8_t)pos;
		}
		fen++;
		rule50=atoi(fen); // jak dlouho se nebralo nebo nehralo pescem, v plies
		while(*fen++!=' ');
		b->move=(int16_t)((atoi(fen)-1)*2);
		if(b->side==BLACK) b->move++; // move je pocet jiz odehranych plies - pocita se od 0
		b->move_start=b->move; //kolik plies nemam ulozeno, uz bylo odehrano drive
		b->rule50move=(int16_t)(b->move-rule50); // move kde se naposledy bralo nebo hralo pescem, plies
		//b->move-b->move_start slouzi jako index ulozenych pozic - 0 prvni pozice na zacatku, 1 - pozice po prvnim pultahu,
		// 2 po druhem pultahu, atd


		collect_material_from_board(b, &pw, &pb, &nw, &nb, &bwl, &bwd, &bbl, &bbd, &rw, &rb, &qw, &qb);

#if 0
		bwl=bwd=bbl=bbd=0;
		bwl= BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE])& WHITEBITMAP);
		bwd= BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE])& BLACKBITMAP);
		bbl= BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK])& WHITEBITMAP);
		bbd= BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK])& BLACKBITMAP);
#endif

#if 0
		b->material[WHITE][DBISHOP]=(uint8_t)bwd;
		b->material[BLACK][DBISHOP]=(uint8_t)bbd;
#endif

#if 0
		b->mindex=MATidx(b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
					b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
					b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
#endif
		b->mindex=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
					

		b->mindex_validity=0;
		check_mindex_validity(b, 1);
		setupRandom(b);
		setupPawnRandom(b);
}


void writeEPD_FEN(board *b, char *fen, int epd, char *option){
int x,y,i, from, e;
char c, f[100];
	sprintf(fen,"%s","");
	for(y=7;y>=0;y--){
		i=0;
		f[0]='\0';
		for(x=0;x<=7;x++){
			i++;
			from=y*8+x;
			if(b->pieces[from]!=ER_PIECE) {
				c=PIECES_ASC[b->pieces[from]&PIECEMASK];
				if(b->pieces[from] & BLACKPIECE) c=(char)tolower(c);
				if(i>1) {
					sprintf(f,"%d",i-1);
					strcat(fen, f);
				}
				i=0;
				sprintf(f,"%c",c);
				strcat(fen, f);
			}
		}
		if(i>1) {
			sprintf(f,"%d",i);
			strcat(fen, f);
		}
		if(y!=0) strcat(fen, "/");
	}
	if(b->side==BLACK) sprintf(f," b"); else sprintf(f," w");
	strcat(fen, f);
	if((b->castle[WHITE]==0) && (b->castle[BLACK]==0)) 	strcat(fen, " -");
	else {
		strcat(fen, " ");
		switch(b->castle[WHITE]) {
		case QUEENSIDE:
			strcat(fen, "Q");
			break;
		case KINGSIDE:
			strcat(fen, "K");
			break;
		case BOTHSIDES:
			strcat(fen, "KQ");
			break;
		}
		switch(b->castle[BLACK]) {
		case QUEENSIDE:
			strcat(fen, "q");
			break;
		case KINGSIDE:
			strcat(fen, "k");
			break;
		case BOTHSIDES:
			strcat(fen, "kq");
			break;
		}
	}
	if(b->ep!=-1) {
		e=b->ep;
		if(b->side==WHITE) e+=8; else e-=8;
		sprintf(f, " %c%d", 'a'+e%8, e/8+1);
	} else sprintf(f, " -");
	strcat(fen, f);
	if(epd!=0) {
		sprintf(f," ; id %s", option);
		strcat(fen, f);
	} else {
		sprintf(f," %d %d", b->move-b->rule50move, b->move/2+1);
		strcat(fen, f);
	}
}

void setup_normal_board(board *b){
	setup_FEN_board(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void printboard(board *b) 
{
//	printf("Side: %s, EP: %d, ply: %d, rule50: %d\n", (b->side==WHITE) ? "White" : "Black", b->ep, b->move, b->rule50move);
	printmask(b->norm, "Normal");
	printmask(b->maps[KING], "Kings");
	printmask(b->maps[QUEEN], "Queens");
	printmask(b->maps[BISHOP], "Bishops");
	printmask(b->maps[KNIGHT], "Knights");
	printmask(b->maps[ROOK], "Rooks");
	printmask(b->maps[PAWN], "Pawns");
	printmask(b->colormaps[WHITE], "White");
	printmask(b->colormaps[BLACK], "Black");
}
