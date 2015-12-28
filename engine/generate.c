
#include "bitmap.h"
#include "evaluate.h"
#include "hash.h"
#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include "search.h"
#include "globals.h"

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

//		for(n=0;n<64;n++) {
//			printmask(norm[n]);
//		}
}

void generate_w_pawn_attack(BITVAR norm[])
{
int f,n,r,x,y;

int moves[]= { -1,1, 1,1 };	

	
BITVAR q1;
	
	q1 = EMPTYBITMAP;

	for(n=0;n<8;n++) {
//			norm[n]=EMPTYBITMAP;		
			norm[56+n]=EMPTYBITMAP;
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

//	for(n=0;n<64;n++) {
//			printmask(norm[n]);
//	}
}


void generate_b_pawn_moves(BITVAR norm[])
{
int f,n;
	
BITVAR q1;
	
	for(n=0;n<8;n++) {
			norm[n]=EMPTYBITMAP;		
			norm[56+n]=EMPTYBITMAP;
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
//	for(n=0;n<64;n++) {
//			printmask(norm[n]);
//	}
}

void generate_b_pawn_attack(BITVAR norm[])
{
int f,n,r,x,y;

int moves[]= { -1,-1, 1,-1 };	

	
BITVAR q1;
	
	q1 = EMPTYBITMAP;

	for(n=0;n<8;n++) {
			norm[n]=EMPTYBITMAP;		
//			norm[56+n]=EMPTYBITMAP;
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

//	for(n=0;n<64;n++) {
//			printmask(norm[n], "BLACK PAWN");
//	}
}

void generate_topos()
{
int n,r;
char f;	
		n=0;
		for(f=0;f<=16;f++) {
			r=1<<f;
			while(n<r) {
//				printf("%d %d\n",n,f-1);
				ToPos[n++]=f-1;
			}
		}
}

void generate_rays()
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
//		for(f=1;f<64;f+=8) 
//			for(n=0;n<64;n++) {
//							printf("%d %d\n",f,n);
//							printmask(rays[f][n]);
//			}

}

void generate_w_passed_pawn_mask(BITVAR map[64])
{
int x,y;
BITVAR m;	
	for(x=0;x<64;x++) map[x]=EMPTYBITMAP;
	for(x=0;x<8;x++) {
	    for(y=1;y<6;y++) {
		    m=rays[(y+1)*8+x][A7+x];
		    if(x>0) m|=rays[(y+1)*8+x-1][A7+x-1];
		    if(x<7) m|=rays[(y+1)*8+x+1][A7+x+1];
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
	    for(y=2;y<7;y++) {
			m=0;
		    if(x>0) m|=rays[(y-1)*8+x-1][A2+x-1];
		    if(x<7) m|=rays[(y-1)*8+x+1][A2+x+1];
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
	    for(y=2;y<7;y++) {
		    m=rays[(y-1)*8+x][A2+x];
		    if(x>0) m|=rays[(y-1)*8+x-1][A2+x-1];
		    if(x<7) m|=rays[(y-1)*8+x+1][A2+x+1];
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
	    for(y=2;y<7;y++) {
		    m=0;
		    if(x>0) m|=rays[(y+1)*8+x-1][A7+x-1];
		    if(x<7) m|=rays[(y+1)*8+x+1][A7+x+1];
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
	    	m= rays[A8+x][x];
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
	for(x=0;x<8;x++) {
	    for(y=0;y<8;y++) {
	    	m= rays[y*8][y*8+H1];
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
		if(x>0) m|=rays[A8+x-1][x-1];
		if(x<7) m|=rays[A8+x+1][x+1];
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


void generate_uphalf(BITVAR map[64], att_mov att)
{
BITVAR b;
int x,y,z;
	
	b=EMPTYBITMAP;
	for(x=0;x<8;x++) map[x]=b;
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
	for(x=0;x<8;x++) map[x]=b;
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
	for(y=0;y<8;y++) map[y*8]=b;
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
	for(y=0;y<8;y++) map[y*8]=b;
	for(x=0;x<7;x++) {
		b=EMPTYBITMAP;
		for(z=x+1;z<8;z++) {
			b|=att.file[z];
		}
		for(y=0;y<8;y++) map[y*8+x]=b;
	}
//	for(z=0;z<64;z++) printmask(map[z], "Right\n");
}

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

void generate_attack_r45R(BITVAR map[64][256])
{
int xu,yu,xl,yl,z,f,n,l,u,zt;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {			
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound				
					u=1U<<ind45R[f*8+n];
					zt=z & ~u;
					while((xu<7) && (yu<7) && (u<128)) {
						if(zt & u) break;
						u<<=1;
						yu++;
						xu++;
					}
// lower bound				
					xl=n;
					yl=f;
					l=1U<<ind45R[f*8+n];;
					zt=z & ~l;
					while((xl>=0)  && (yl>=0) && (l>1)) {
						if(zt & l) break;
						l>>=1;
						yl--;
						xl--;
					}
// find 	proper ray 
					map[f*8+n][z]=rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind45R[f*8+n]);
//					printmask(map[f*8+n][z]);
				}
			}
		}
}

void generate_attack_r45L(BITVAR map[64][256])
{
int xu,yu,xl,yl,z,f,n,l,u,zt;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {			
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound				
					u=1U<<ind45L[f*8+n];
					zt=z & ~u;
					while((xu>0)  && (yu<7) && (u<128)) {
						if(zt & u) break;
						u<<=1;
						yu++;
						xu--;
					}
// lower bound				
					xl=n;
					yl=f;
					l=1U<<ind45L[f*8+n];;
					zt=z & ~l;
					while((xl<7) && (yl>0)  && (l>1)) {
						if(zt & l) break;
						l>>=1;
						yl--;
						xl++;
					}
// find 	proper ray 
					map[f*8+n][z]=rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind45L[f*8+n]);
//					printmask(map[f*8+n][z]);
				}
			}
		}
}

void generate_attack_r90R(BITVAR map[64][256])
{
int xu,yu,xl,yl,z,f,n,l,u,zt;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {			
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound				
					u=1U<<ind90[f*8+n];
					zt=z & ~u;
					while((yu<7) && (u<128)) {
						if(zt & u) break;
						u<<=1;
						yu++;
					}
// lower bound				
					xl=n;
					yl=f;

					l=1U<<ind90[f*8+n];
					zt=z & ~l;
					while((yl>=0) && (l>1)) {
						if(zt & l) break;
						l>>=1;
						yl--;
					}
// find 	proper ray 
					map[f*8+n][z]=rays[yl*8+xl][yu*8+xu];
					map[f*8+n][z]=ClrNorm(f*8+n,map[f*8+n][z]);
//					printf("f=%d n=%d xu=%d yu=%d xl=%d yl=%d z=%d rot=%d\n", f,n,xu, yu, xl, yl, z, ind90[f*8+n]);
//					printmask(map[f*8+n][z],"!!!");
				}
			}
		}
}

void generate_attack_norm(BITVAR map[64][256])
{
int xu,yu,xl,yl,z,f,n,l,u,zt;
	
		for(f=0;f<8;f++) {
			for(n=0;n<8;n++) {
				for(z=0;z<256;z++) {
					xu=n;
					yu=f;

// upper bound				
					u=1U<<indnorm[f*8+n];;
					zt=z & ~u;
					while((xu<7) && (u<128)) {
						if(zt & u) break;
						u<<=1;
						xu++;
					}
// lower bound				
					xl=n;
					yl=f;
					l=1U<<indnorm[f*8+n];;
					zt=z & ~l;
					while((xl>=0)  && (l>1)) {
						if(zt & l) break;
						l>>=1;
						xl--;
					}
// find 	proper ray 
					map[f*8+n][z]=rays[yl*8+xl][yu*8+xu];
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
//		b->mcount[WHITE]=0;

//		b->mcount[BLACK]=0;
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

		b->castle[WHITE]=0;
		b->castle[BLACK]=0;
		b->move=0;
		b->rule50move=0;
		b->key=0;
		b->gamestage=OPENING;
}

void setup_normal_board2(board *b)
{
int f;		
		empty_board(b);
	
// pawns
	
		for(f=A2;f<=H2;f++) {
			SetAll(f, WHITE, PAWN, b);
		}
		for(f=A7;f<=H7;f++) {
			SetAll(f, BLACK, PAWN, b);
		}
			
// kings		
		SetAll(E1, WHITE, KING, b);
		SetAll(E8, BLACK, KING, b);
		b->king[WHITE]=E1;
		b->king[BLACK]=E8;

// queens
		SetAll(D1, WHITE, QUEEN, b);
		SetAll(D8, BLACK, QUEEN, b);
		
// rooks
		SetAll(A1, WHITE, ROOK, b);
		SetAll(A8, BLACK, ROOK, b);
		SetAll(H1, WHITE, ROOK, b);
		SetAll(H8, BLACK, ROOK, b);

// bishops
		SetAll(C1, WHITE, BISHOP, b);
		SetAll(C8, BLACK, BISHOP, b);
		SetAll(F1, WHITE, BISHOP, b);
		SetAll(F8, BLACK, BISHOP, b);
		
// knights
		SetAll(B1, WHITE, KNIGHT, b);
		SetAll(B8, BLACK, KNIGHT, b);
		SetAll(G1, WHITE, KNIGHT, b);
		SetAll(G8, BLACK, KNIGHT, b);


//		SetAll(E7, WHITE, PAWN, b);

		b->ep=-1;
		b->side=WHITE;	// white
//		b->mcount[WHITE]=0;
//		b->mcount[BLACK]=0;
		b->material[WHITE][PAWN]=8;
		b->material[WHITE][QUEEN]=1;
		b->material[WHITE][BISHOP]=2;
		b->material[WHITE][ROOK]=2;
		b->material[WHITE][KNIGHT]=2;
		b->material[WHITE][KING]=1;

		b->material[BLACK][PAWN]=8;
		b->material[BLACK][QUEEN]=1;
		b->material[BLACK][BISHOP]=2;
		b->material[BLACK][ROOK]=2;
		b->material[BLACK][KNIGHT]=2;
		b->material[BLACK][KING]=1;
		b->mindex=MATidx(8,8,2,2,1,1,1,1,2,2,1,1);
		b->mindex2=MATidx2(8,8,2,2,1,1,1,1,2,2,1,1);
		
		setupRandom(b);
}

void setup_FEN_board(board *b, char * fen)
{
int pos, val, x,y;
int bwl, bwd, bbl, bbd;
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
   field 5: halfmove clock - count of plys since pawn or capture
   field 6: fullmove - number of moves
*/


		empty_board(b);
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
			else if(islower(*fen)) {	

				switch (*fen) {
					case 'k' : SetAll(y*8+x, BLACK, KING, b);
//								  b->material[BLACK][KING]++;
//								  b->mcount[BLACK]+=Values[KING];
								  b->king[BLACK]=y*8+x;
								break;

					case 'n' : SetAll(y*8+x, BLACK, KNIGHT, b);
//								  b->material[BLACK][KNIGHT]++;
//								  b->mcount[BLACK]+=Values[KNIGHT];
								break;
					case 'q' : SetAll(y*8+x, BLACK, QUEEN, b);
//								  b->material[BLACK][QUEEN]++;
//								  b->mcount[BLACK]+=Values[QUEEN];
								break;
					case 'r' : SetAll(y*8+x, BLACK, ROOK, b);
//								  b->material[BLACK][ROOK]++;
//								  b->mcount[BLACK]+=Values[ROOK];
								break;
					case 'p' : SetAll(y*8+x, BLACK, PAWN, b);
//								  b->material[BLACK][PAWN]++;
//								  b->mcount[BLACK]+=Values[PAWN];
								break;
					case 'b' : SetAll(y*8+x, BLACK, BISHOP, b);
//								  b->material[BLACK][BISHOP]++;
//								  b->mcount[BLACK]+=Values[BISHOP];
								break;
					default : printf("ERROR!\n");
				}
				fen++;
				x++;
			} 
			
			else {
				switch (*fen) {
					case 'K' : SetAll(y*8+x, WHITE, KING, b);
//								  b->material[WHITE][KING]++;
//								  b->mcount[WHITE]+=Values[KING];
								  b->king[WHITE]=y*8+x;
								break;
					case 'N' : SetAll(y*8+x, WHITE, KNIGHT, b);
//								  b->material[WHITE][KNIGHT]++;
//								  b->mcount[WHITE]+=Values[KNIGHT];
								break;
					case 'Q' : SetAll(y*8+x, WHITE, QUEEN, b);
//								  b->material[WHITE][QUEEN]++;
//								  b->mcount[WHITE]+=Values[QUEEN];
								break;
					case 'R' : SetAll(y*8+x, WHITE, ROOK, b);
//								  b->material[WHITE][ROOK]++;
//								  b->mcount[WHITE]+=Values[ROOK];
								break;
					case 'P' : SetAll(y*8+x, WHITE, PAWN, b);
//								  b->material[WHITE][PAWN]++;
//								  b->mcount[WHITE]+=Values[PAWN];
								break;
					case 'B' : SetAll(y*8+x, WHITE, BISHOP, b);
//								  b->material[WHITE][BISHOP]++;
//								  b->mcount[WHITE]+=Values[BISHOP];
								break;
					default : printf("ERROR!\n");
				}
				fen++;
				x++;
			}
		}
// material counts
		b->material[WHITE][PAWN]= BitCount((b->colormaps[WHITE])&(b->maps[PAWN]));
		b->material[WHITE][KNIGHT]= BitCount((b->colormaps[WHITE])&(b->maps[KNIGHT]));
		b->material[WHITE][BISHOP]= BitCount((b->colormaps[WHITE])&(b->maps[BISHOP]));
		b->material[WHITE][ROOK]= BitCount((b->colormaps[WHITE])&(b->maps[ROOK]));
		b->material[WHITE][QUEEN]= BitCount((b->colormaps[WHITE])&(b->maps[QUEEN]));
		b->material[WHITE][KING]= BitCount((b->colormaps[WHITE])&(b->maps[KING]));

		b->material[BLACK][PAWN]= BitCount((b->colormaps[BLACK])&(b->maps[PAWN]));
		b->material[BLACK][KNIGHT]= BitCount((b->colormaps[BLACK])&(b->maps[KNIGHT]));
		b->material[BLACK][BISHOP]= BitCount((b->colormaps[BLACK])&(b->maps[BISHOP]));
		b->material[BLACK][ROOK]= BitCount((b->colormaps[BLACK])&(b->maps[ROOK]));
		b->material[BLACK][QUEEN]= BitCount((b->colormaps[BLACK])&(b->maps[QUEEN]));
		b->material[BLACK][KING]= BitCount((b->colormaps[BLACK])&(b->maps[KING]));
		
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
					LOGGER_1("ERR:", "Castling: Black King to Queen side problem!","\n");
					break;
				}
				b->castle[BLACK]+=QUEENSIDE;
				break;
			case 'Q' :
				if((b->pieces[E1]!=(KING)) || (b->pieces[A1]!=(ROOK))) {
					LOGGER_1("ERR:", "Castling: White King to Queen side problem!","\n");
					break;
				}
				b->castle[WHITE]+=QUEENSIDE;
				break;
			case 'k' :
				if((b->pieces[E8]!=(KING|BLACKPIECE)) || (b->pieces[H8]!=(ROOK|BLACKPIECE))) {
					LOGGER_1("ERR:", "Castling: Black King to King side problem!","\n");
					break;
				}
				b->castle[BLACK]+=KINGSIDE;
				break;
			case 'K' :
				if((b->pieces[E1]!=(KING)) || (b->pieces[H1]!=(ROOK))) {
					LOGGER_1("ERR:", "Castling: White King to King side problem!","\n");
					break;
				}
				b->castle[WHITE]+=KINGSIDE;
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
			fen+=2;
		} 
		else {
			pos=(toupper((*fen++)) - 'A') ;
			pos+=(((*fen++)-'1')*8);
			if(b->side==WHITE) pos-=8; else pos+=8;
			b->ep=pos;
		}
		fen++;
		b->rule50move=atoi(fen);
		while(*fen++!=' ');
		b->move=(atoi(fen));

		bwl=bwd=bbl=bbd=0;
		bwl=BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE])& WHITEBITMAP);
		bwd=BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE])& BLACKBITMAP);
		bbl=BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK])& WHITEBITMAP);
		bbd=BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK])& BLACKBITMAP);
		
		b->material[WHITE][ER_PIECE+BISHOP]=bwd;
		b->material[BLACK][ER_PIECE+BISHOP]=bbd;
		
		b->mindex=MATidx(b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
					b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
					b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
					
		b->mindex2=MATidx2(b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
					b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
					b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
		
		setupRandom(b);
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
				if(b->pieces[from] & BLACKPIECE) c=tolower(c);
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
		sprintf(f," %d %d", b->rule50move, b->move);
		strcat(fen, f);
	}
}
void setup_normal_board(board *b){
	setup_FEN_board(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void printboard(board *b) 
{
	printf("Side: %s, EP: %d, ply: %d, rule50: %d\n", (b->side==WHITE) ? "White" : "Black", b->ep, b->move, b->rule50move);
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

void setup_options()
{
	options.zeromove=1;
	options.killers=1;
	options.quiesce=1;
	options.hash=1;
	options.alphabeta=1;
}
