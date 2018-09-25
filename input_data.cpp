#include <iostream>
#include <fstream>
#include <string>
#include <numeric>
#include <limits>
#include <cassert>
#include <string.h>
#include "types.h"
#include "input_data.h"

using namespace std;

extern bool start_index_is_zero;

void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void print(int num_vertices, degree_t *degree, vertex_t ** adj_list)
{
    // print degree
    for (int i = 0; i < num_vertices; ++i)
    {
        cout << i << " " << degree[i] << endl;
    }
    cout << endl;

    // print adj_list
    for (int i = 0; i < num_vertices; ++i)
    {
        cout << i << " : ";
        for (int j = 0; j < degree[i]; ++j)
        {
            cout << adj_list[i][j] << " ";
        }
        cout << endl;
    }
}
void testReadData()
{
    degree_t * degree = nullptr;
    vertex_t ** adj_list = nullptr;
    int num_vertices = 0;
    int num_edges = 0;
    start_index_is_zero = false;
    readData("../data/web-Stanford.txt", &degree, &adj_list, &num_vertices, &num_edges);
    //readData("../data/test.txt", &degree, &adj_list, &num_vertices, &num_edges);
    cout << "num_vertices=" << num_vertices << ", " << "num_edges=" << num_edges << endl;
    vertex_t * row_ptr = (vertex_t*)malloc(sizeof(vertex_t) * (num_vertices + 1));
    vertex_t * col = (vertex_t*)malloc(sizeof(vertex_t) * num_edges);
    consCSR(num_vertices, degree, adj_list, row_ptr, col);

    // check
    for (size_t i = 0 ; i < num_vertices; ++i)
    {
        for (size_t j = row_ptr[i]; j < row_ptr[i+1]; ++j)
        {
            assert(col[j] == adj_list[i][j-row_ptr[i]]);
        }
    }
    free(row_ptr);
    free(col);
    releaseData(num_vertices, degree, adj_list);
}

inline void zeros(void * m, int size)
{
    memset(m, 0, size);
}

void readData(const char * filename, degree_t ** degree_p, vertex_t *** adj_list_p, int * num_vertices_p, int * num_edges_p)
{
    ifstream fin(filename);
    char c;
    string str;
    char s[1000];
    int num_vertices;
    int num_edges;
    // first read all data to get degrees of each node, so we can allocate the memory of adjacency list
    while((c = fin.peek()) == '#')
    {
        fin.get(); // extract '#'
        fin >> str;
        if (str == string("Nodes:"))
        {
            fin >> num_vertices;
            fin >> str;
            fin >> num_edges;
        }
        // jump to next line
        fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // allocate memory to degree
    degree_t * degree = (degree_t*)malloc(sizeof(degree_t) * num_vertices);
    zeros(degree, sizeof(degree_t) * num_vertices);

    // remember the position
    int pos = fin.tellg();

    vertex_t src, dst;
    int count = 0;
    for (int i = 0; i < num_edges; ++i)
    {
        fin >> src;
        fin >> dst;
        if (!start_index_is_zero) {
            src--;
            dst--;
        }
        if (src == dst)
            continue;
        if (src > dst)
            swap(&src, &dst);
        degree[src]++;
        if (++count % 10000000 == 0)
            cout << "";
            //cout << count << endl;
    }
    fin.close();

    vertex_t ** adj_list = (vertex_t**)malloc(sizeof(vertex_t*) * num_vertices);
    zeros(adj_list, sizeof(vertex_t*) * num_vertices);
    for (size_t i = 0; i < num_vertices; ++i)
    {
        adj_list[i] = (vertex_t*)malloc(sizeof(vertex_t) * degree[i]);
        zeros(adj_list[i], sizeof(vertex_t) * degree[i]);
    }

    // reset position to the position that remenbered before
    fin.open(filename);
    fin.seekg(pos);

    // a array to record the number of edges that each vertex has read
    int * counter = (int*)malloc(sizeof(int) * num_vertices);
    zeros(counter, sizeof(int) * num_vertices);
    count=0;
    for (int i = 0; i < num_edges; ++i)
    {
        fin >> src;
        fin >> dst;
        if (!start_index_is_zero) {
            src--;
            dst--;
        }
        if (src == dst)
            continue;
        if (src > dst)
            swap(&src, &dst);
        adj_list[src][counter[src]] = dst;
        counter[src]++;
        if (++count % 10000000 == 0)
           //cout << count << endl;
            cout << "";
    }

    // release useless memory
    free(counter);

    *degree_p = degree;
    *adj_list_p = adj_list;
    *num_vertices_p = num_vertices;
    *num_edges_p = count;
}

void consSrcsOfEdges(int num_vertices, int * degree, int * srcs_of_edges)
{
    int index = 0;
    for (int i = 0; i < num_vertices; ++i)
    {
        for (int j = 0; j < degree[i]; ++j)
            srcs_of_edges[index++] = i;
    }
}

void consCSR(int num_vertices, degree_t * degree, vertex_t ** adj_list, vertex_t * row_ptr, vertex_t * col)
{
    row_ptr[0] = 0;
    for (size_t i = 0; i < num_vertices; ++i)
    {
        row_ptr[i+1] = row_ptr[i] + degree[i];
        vertex_t start = row_ptr[i];
        vertex_t * dst_list = adj_list[i];
        for (size_t j = 0; j < degree[i]; ++j)
        {
            col[start+j] = dst_list[j];
        }
    }
}

void consCSC(int num_vertices, degree_t * out_degree, vertex_t ** adj_list, vertex_t * col_ptr, vertex_t * row)
{
    int * in_degree = (int*)malloc(sizeof(int) * num_vertices);
    zeros(in_degree, sizeof(int) * num_vertices);
    // get in degree of each vertex
    for (int i = 0; i < num_vertices; ++i)
    {
        vertex_t * dst_list = adj_list[i];
        for (int j = 0; j < out_degree[i]; ++j)
        {
            in_degree[dst_list[j]]++;
        }
    }

    int * ind = (int*)malloc(sizeof(int) * num_vertices);
    zeros(ind, sizeof(int) * num_vertices);
    for (int i = 1; i < num_vertices; ++i)
    {
        ind[i] = ind[i-1] + in_degree[i-1];
    }
    for (int i = 0; i < num_vertices; ++i)
    {
        vertex_t * dst_list = adj_list[i];
        for (int j = 0; j < out_degree[i]; ++j)
        {
            row[ind[dst_list[j]]++] = i;
        }
    }
    col_ptr[0] = 0;
    for (int i = 1; i <= num_vertices; ++i)
    {
        col_ptr[i] = col_ptr[i-1] + in_degree[i-1];
    }

    //check
    for (int i = 0; i < num_vertices; ++i)
    {
        assert(col_ptr[i+1] == ind[i]);
    }
}

void releaseData(int num_vertices, degree_t * degree, vertex_t ** adj_list)
{
    for (int i = 0; i < num_vertices; ++i)
    {
        free(adj_list[i]);
    }
    free(degree);
    free(adj_list);
}
