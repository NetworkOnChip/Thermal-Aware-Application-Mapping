#include<stdio.h>
#include<time.h>

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <string.h>
using namespace std;

#define max_router 1024
#define max_edge 2000
#define max_core 1024
#define max_path 1000
#define max_node 1024
#define max_layer 8
#define s_flit 32
#define s_packet 4
#define INF 9999999
#define overhead 1.2
#define absurd 9999999
#define white 100002
#define gray 100004
#define black 100006
/////////////////////////////////
//pso related macros
#define no_seed 5000
#define max_gen 1000
//pso related variables
int prob_g = 9;
int prob_l = 9;
int prob_r = 5;
////////////////////////////////
int ss_g_best[max_core];
int ss_l_best[max_core];
int unchanged = 0;

//z_mesh related variables
int adj_list[max_node][6]; //for z_mesh there are only max 6 neighbours
int adj_no[max_node]; //number of adjacent nodes for each router
int label[max_router];
int color[max_router];


int x_dim,y_dim,z_dim;
int Num_Router;
char file1[100] = "task_graph_VOPD.txt";
char file2[100] = "Output_file.txt";
char file3[100] = "hop_count_file.txt";
char file4[100] = "mapping_file.txt";

///read core grpah and update the following arrays
float core_graph[max_core][max_core];
int Num_Core;
int Num_Edge;
float BW[max_edge]; // in MBps
float BWMP[max_edge]; //in pps
float tot_com;
int src_c[max_edge];
int dst_c[max_edge];
//communication cost
float cc_min;
int hop_count[max_router][max_router];


struct node
{	
	int x_cord,y_cord,z_cord;
	int r; //the router number
	int c; //the core number
}n[max_router];

struct particle
{
	//for current particle
	int mapping[max_core];
	float cc;
	////for local best particle
	int best_mapping[max_core];
	float best_cc;
}p[no_seed];

struct global_best
{
	int best_mapping[max_core];
	float best_cc;
}g;


int difference(int x, int y)
{
	if(x>y)
		return((x-y));
	else
		return((y-x));
}

int min_of(int a, int b)
{
	if(a<=b)
		return a;
	else
		return b; 
}

int randInRange1(int min, int max)
{
	
	if(max > min)
  		return (min + (rand() % (max - min + 1)));
	else
		return 0;
}

void find_cord(int i)
{		
	int eq_n;
	
	n[i].z_cord=i/(x_dim*y_dim);
	eq_n=i-n[i].z_cord*(x_dim*y_dim);
	n[i].y_cord=eq_n/x_dim;
	n[i].x_cord=eq_n-(n[i].y_cord*x_dim);
}

void initialize_nodes()
{
	int i;
	for(i=0;i<Num_Router;i++)
	{
		n[i].r = i;
		find_cord(i);
	}
}
				
		
void init_core_graph()
{
	int i,j;
	for(i=0;i<max_core;i++)
	{
		for(j=0;j<max_core;j++)
		{
			core_graph[i][j] = INF;
		}
	}
	
}

void read_task_graph()
{
	int i,j,e=0;
	char temp_core_count[100],temp_src[100],temp_dst[100],temp_BW[100];
	FILE *f;
	f = fopen(file1,"r");
	fscanf(f,"%s",temp_core_count);
	Num_Core = atoi(temp_core_count);
	cout << Num_Core <<" Num_Core"<<endl;
 	for(i=0;i<Num_Core;i++){
 		for(j=0;j<Num_Core;j++){
 			core_graph[i][j]=INF;
 			if(i==j)core_graph[i][j]=0; 
 		}
    }
	while(getc(f)!=EOF){
				fscanf(f,"%s",temp_src);
				fscanf(f,"%s",temp_dst);
				fscanf(f,"%s",temp_BW);
				core_graph[i][j] = atof(temp_BW);
				src_c[e] = atoi(temp_src); 
				dst_c[e] = atoi(temp_dst);
				BW[e] = core_graph[i][j]; // BW in MBps
				BWMP[e] = BW[e]/ (s_packet * s_flit ) ;//mpps      (remove 1024*1024 for mpps)
				cout<<"i"<<temp_src<<"j"<<temp_dst<<"BW[e]"<<BW[e]<<endl;
				e++;
	}
	/*for(i=0;i<Num_Core;i++)
		{
			for(j=0;j<Num_Core;j++)
			{
				cout<<"i"<<i<<"j"<<j<<"core_graph[i][j]"<<core_graph[i][j]<<endl;
			}
		}*/
	Num_Edge = e;
	printf("Number of edges in the Graph are %d\n",Num_Edge);
	fclose(f);
}

void init_adj_list()
{
	int i,j;
	for(i=0;i<Num_Router;i++)
	{
		for(j=0;j<6;j++)
		{
			adj_list[i][j] = absurd;
		}
		adj_no[i] = 0;
	}
}

void create_adj_list()
{
	int i,j,k;
	init_adj_list();
	for(i=0;i<Num_Router;i++)
	{
		//up adjacent
		if(n[i].y_cord != (y_dim - 1))
		{
			adj_list[i][adj_no[i]] = i+x_dim;
			adj_no[i] = adj_no[i] + 1;
		}	
		//down adjacent
		if(n[i].y_cord != 0)
		{
			adj_list[i][adj_no[i]] = i-x_dim;
			adj_no[i] = adj_no[i] + 1;
		}	
		//left adjacent
		if(n[i].x_cord != 0)
		{
			adj_list[i][adj_no[i]] = i - 1;
			adj_no[i] = adj_no[i] + 1;
		}	
		//right adjacent
		if(n[i].x_cord != (x_dim - 1))
		{
			adj_list[i][adj_no[i]] = i + 1;
			adj_no[i] = adj_no[i] + 1;
		}	
			
		//diagonally up adjacent
		if((n[i].y_cord%2 == 0) && (n[i].x_cord != x_dim -1) && ((i+x_dim+1) < Num_Router)) //for even rows : up adjacent is at right side
		{
			adj_list[i][adj_no[i]] = i+x_dim+1;
			adj_no[i] = adj_no[i] + 1;
		}	
		//diagonally up adjacent
		if((n[i].y_cord%2 != 0) && (n[i].x_cord != 0) && ((i+x_dim-1) < Num_Router)) //for odd rows : up adjacent is at left side
		{
			adj_list[i][adj_no[i]] = i+x_dim-1;
			adj_no[i] = adj_no[i] + 1;
		}	
		//diagonally down adjacent
		if((n[i].y_cord%2 == 0) && (n[i].x_cord != x_dim -1) && ((i-x_dim+1) >= 0)) //for even rows : down adjacent is at right side
		{
			adj_list[i][adj_no[i]] = i-x_dim+1;
			adj_no[i] = adj_no[i] + 1;
		}	
		//diagonally down adjacent
		if((n[i].y_cord%2 != 0) && (n[i].x_cord != 0) && ((i-x_dim-1) >= 0)) //for odd rows : down adjacent is at left side
		{
			adj_list[i][adj_no[i]] = i-x_dim-1;						
			adj_no[i] = adj_no[i] + 1;
		}	
	}
	for(i=0;i<Num_Router;i++)
	{
		if(adj_no[i] > 6)
		{
			printf("*************ERROR*************\n");
			return;
		}
		printf("node_%d\n",i);
		for(j=0;j<adj_no[i];j++)
		{
			printf("%d\t",adj_list[i][j]);
		}
		printf("\n");
	}
}

void init_label()
{
	int i;
	
	for(i=0;i<Num_Router;i++)
	{
		label[i] = absurd;
	}
}

void init_color()
{
	int i;
	for(i=0;i<Num_Router;i++)
	{
		color[i] = white;
	} 
}

int fifo_out(int* queue, int ptr)
{
	int i;
	for(i=0;i<=ptr-1;i++)
	{
		queue[i]=queue[i+1];
	}
	ptr--;
	return(ptr);
}

int breadth_first_search_forward_numbering(int src,int dst)
{
	int length,node,dest_found,adj_node,count=0,j,i;
	int queue[max_router],ptr = 0;
	printf("\nsrc = %d, dst = %d",src,dst);
	init_label();
	init_color();
	for(i=0;i<Num_Router;i++)
		queue[i]=0;
	ptr = 0;	
	label[src] = 0;
	color[src] = gray;
	queue[ptr] = src;
	while(ptr >= 0) 
	{
		count++;
		node = queue[0];
		ptr = fifo_out(queue,ptr);
		
		dest_found = 0;
		for(i=0;i<6;i++)
		{
			adj_node = adj_list[node][i];
			if(adj_node != absurd)
			{
				if(color[adj_node] == white)
				{
					if(adj_node == dst)
					{
						dest_found = 1;
						length = label[node] + 2;
					}
					else
					color[adj_node] = gray;
					label[adj_node] = label[node] + 1;
					ptr = ptr+1;
					queue[ptr] = adj_node;
				}
				
			}
		}
		color[node] = black;
		if(dest_found == 1)
			break;
	}
	//printf("\tHop_Count = %d\n",length);
	return(length);
}

void determine_hop_count(int net_id)
{
	int i,j,k;
	if(net_id ==1) //for mesh net
	{
		for(i=0;i<Num_Router;i++)
		{
			for(j=0;j<Num_Router;j++)
			{
				hop_count[i][j] = difference(n[i].x_cord,n[j].x_cord) + difference(n[i].y_cord,n[j].y_cord) + difference(n[i].z_cord,n[j].z_cord);
			}
		}
	}
	if(net_id == 2) //for z_mesh
	{
		create_adj_list();
		//breadth_first_search_forward_numbering(src,dst);
		for(i=0;i<Num_Router;i++)
		{
			for(j=0;j<Num_Router;j++)
			{
				if(i!=j)
					hop_count[i][j] = breadth_first_search_forward_numbering(i,j) - 1;
				else
					hop_count[i][j] = 0;
					
				printf("\tHop_Count = %d\n",hop_count[i][j]);		
			}	
		}	
	}
}

void find_cost_of_mapping(int n1)		
{
	int i;
	p[n1].cc = 0.0;
	for(i=0;i<Num_Edge;i++)
	{
		p[n1].cc = p[n1].cc + BW[i]*hop_count[p[n1].mapping[src_c[i]]][p[n1].mapping[dst_c[i]]];
	}
}

void initialize_particles()
{
	int i,j,k;
	int rtr;
	int valid;
	float best_v = INF;
	int best_p = 0;
	//initialize particle
	for(i=0;i<no_seed;i++)
	{
		if(i!=0)
		{
			for(j=0;j<Num_Core;j++)
			{
				while(1)
				{
					rtr = randInRange1(0,Num_Router-1);
					valid = 1;
					for(k=0;k<j;k++)
					{
						if(p[i].mapping[k] == rtr)
							valid = 0;
					}
					if(valid == 1)
						break;
				}	
				p[i].mapping[j] = rtr;
			}	
		}
		else
		{
			for(j=0;j<Num_Core;j++)
			{
				p[i].mapping[j] = j;
			}
		}
		find_cost_of_mapping(i);
		if(p[i].cc < best_v)
		{
			best_p = i;
			best_v = p[i].cc;
		}	
	}
	//initialize local best values
	for(i=0;i<no_seed;i++)
	{
		for(j=0;j<Num_Core;j++)
		{
			p[i].best_mapping[j] = p[i].mapping[j];
		}
		p[i].best_cc = p[i].cc;
	}
	//initialize global best values
	for(i=0;i<Num_Core;i++)
	{
		g.best_mapping[i] = p[best_p].mapping[i];
	}
	g.best_cc = p[best_p].cc;
	
}

void print_particle_terminal()
{
	int i,j,k;
	for(i=0;i<no_seed;i++)
	{
		printf("particle_%d\n",i);
		printf("Current_Mapping: ");
		for(j=0;j<Num_Core;j++)
		{
			printf("%d\t",p[i].mapping[j]);
		}
		printf("\n");
		printf("Current_Cost: %f\n",p[i].cc);
		printf("Local_Best_Mapping: ");
		for(j=0;j<Num_Core;j++)
		{
			printf("%d\t",p[i].best_mapping[j]);
		}
		printf("\n");
		printf("Local_Best_Cost: %f\n",p[i].best_cc);
	}	
	printf("////////////////////////////////////////////\n");
	printf("Global_Best_Mapping: ");
	for(j=0;j<Num_Core;j++)
	{
		printf("%d\t",g.best_mapping[j]);
	}
	printf("\n");
	printf("Global_Best_Cost: %f\n",g.best_cc);
	printf("////////////////////////////////////////////\n");
}

void comp_swap_seq_loc(int i)
{
	int j,k;
	//comp_swap_seq_x_loc
	for(j=0;j<Num_Core;j++)
	{
		for(k=0;k<Num_Core;k++)
		{
			if(p[i].mapping[j] == p[i].best_mapping[k])
				break;
		}
		if(k!= Num_Core)
			ss_l_best[j]= k;
		else
			ss_l_best[j]= j;	
	}
}

void comp_swap_seq_glob(int i)
{
	
	int j,k;
	//comp_swap_seq_x_glob
	for(j=0;j<Num_Core;j++)
	{
		for(k=0;k<Num_Core;k++)
		{
			if(p[i].mapping[j] == g.best_mapping[k])
				break;
		}
		if(k!=Num_Core)
			ss_g_best[j]= k;
		else
			ss_g_best[j]= j;	
	}
}


void copy_global_best(int p1)
{
	int i;
	for(i=0;i<Num_Core;i++)
	{
		p[p1].mapping[i] = g.best_mapping[i];
		p[p1].best_mapping[i] = g.best_mapping[i];
	}
	p[p1].cc = g.best_cc;
	p[p1].best_cc = g.best_cc;
}

void read_hop_count_file()
{
	int i,j;
	char temp[100];
	FILE *f;
	f=fopen(file3,"r");
	for(i=0;i<Num_Router;i++)
	{
		for(j=0;j<Num_Router;j++)
		{
			fscanf(f,"%s",temp);
			if(i!=j)
				hop_count[i][j] = atoi(temp);
			else
				hop_count[i][j] = 0;	
		}
	}
	fclose(f);
}

void read_mapping_file()
{
	int i,j;
	char temp[100];
	FILE *f;
	f=fopen(file4,"r");
	for(i=0;i<Num_Core;i++)
	{
		fscanf(f,"%s",temp);
		p[0].mapping[i] = atoi(temp);	
	}
	fclose(f);
	find_cost_of_mapping(0);
	printf("Communication Cost for the mapping is :\n%f\n",p[0].cc);
}

int main(int argc, char *argv[])
{	
	int node_number,i,j,k,t,m,n1,m1,m2,m3;
	int path_in_edge;
	int best_p;
	float best_v;
	float best_pso;
	float best_sa;
	int unchanged = 0;
	FILE *f;
	srand((unsigned int)time(NULL));
	x_dim = atoi(argv[1]);
	y_dim = atoi(argv[2]);
	z_dim = atoi(argv[3]);
	int platform =  atoi(argv[4]);
	//int src = atoi(argv[4]);
	//int dst = atoi(argv[5]);
	Num_Router = x_dim*y_dim*z_dim;
	initialize_nodes();
	init_core_graph();
	read_task_graph();
	determine_hop_count(platform); //1 for mesh, 2 for z-mesh
	//read_hop_count_file();
	
	initialize_particles();
	//print_particle_terminal();
	//read_mapping_file();
	for(i=0;i<max_gen;i++)
	{
		//printf("I m here\n");
		///////////////////////////////////////particle_update//////////////////////////////////
		for(j=0;j<no_seed;j++)
		{
			comp_swap_seq_loc(j);
			comp_swap_seq_glob(j);
			m1 = rand() % 10;
			m2 = rand() % 10;
			m3 = rand() % 10;
			if(m1 < prob_g)
			{
				//apply swap sequence for glob best
				for(k=0 ; k<Num_Core/2; k++)
				{
					m = (rand() % (Num_Core));
					t = p[j].mapping[ss_g_best[m]];
					p[j].mapping[ss_g_best[m]] = p[j].mapping[m];
					p[j].mapping[m] = t;
				}
			}
			if(m2 < prob_l)
			{	
				//apply swap sequence for loc best
				for(k=0 ; k<Num_Core/2; k++)
				{
					m = (rand() % (Num_Core));
					t = p[j].mapping[ss_l_best[m]];
					p[j].mapping[ss_l_best[m]] = p[j].mapping[m];
					p[j].mapping[m] = t;
				}
			}	
			if(m3 < prob_r)
			{
				//apply swap sequence for adding randomness
				for(k=0 ; k<Num_Core/2; k++)
				{
					m = (rand() % (Num_Core));
					n1 = (rand() % (Num_Core));
					t = p[j].mapping[m];
					p[j].mapping[m] = p[j].mapping[n1];
					p[j].mapping[n1] = t;
				}
			}	
		}	
		/////////////////////////////////////local_best_update//////////////////////////////////
		best_p = 0;
		best_v = p[0].best_cc;
		for(j=0;j<no_seed;j++)
		{	
			find_cost_of_mapping(j);
			if(p[j].cc < p[j].best_cc)
			{
				p[j].best_cc = p[j].cc;
				for(k=0;k<Num_Core;k++)
				{
					p[j].best_mapping[k] = p[j].mapping[k];
				}
				if(p[j].cc < best_v)
				{
					best_p = j;
					best_v = p[j].cc;
				}
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////global_best_update//////////////////////////////////
		printf("\nGeneration = %d ",i);
		printf("Globalbest cc = %f ",g.best_cc);
		if(p[best_p].best_cc < g.best_cc)
		{
			for(j=0;j<Num_Core;j++)
			{
				g.best_mapping[j] = p[best_p].mapping[j];
			}
			g.best_cc = p[best_p].cc;
			unchanged = 0;
		}	
		////////////////////////////////////////////////////////////////////////////////////////
		
		printf("\nunchanged = %d\n",unchanged);
		unchanged++;
	}
	
	best_pso = g.best_cc;
	copy_global_best(1);
	best_sa = best_pso;
	cout<<"\nPSO_best CC = "<<best_pso<<", PSO+SA_best CC = "<<best_sa<<"\n";
	
	f = fopen(file2,"a");
	fprintf(f,"PSO_best_CC = %f, PSO+SA_best_CC = %f\n",best_pso,best_sa);
	for(i=0;i<Num_Core;i++)
	{
		fprintf(f,"%d\t",p[1].mapping[i]);
	}
	fprintf(f,"\n");
	fclose(f);
	
	return 0;
}	
