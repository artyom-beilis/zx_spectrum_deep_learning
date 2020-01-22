#include <vector>
#include <stdio.h>
#include "half.hpp"
struct p3d {
    p3d(float a=0,float b=0,float c=0) : x(a),y(b),z(c) {}
    float x,y,z;
};

unsigned short float2half(float v)
{
    half_float::half hv(v);
    unsigned short tmp;
    memcpy(&tmp,&hv,2);
    return tmp;
}


int main()
{
    std::vector<p3d> ver;
    std::vector<std::pair<int,int> > edges;
    ver.push_back(p3d(0,0,-1.0));
    ver.push_back(p3d(0,0,1.0));
    int N=3;
    int as=30;
    int M=360/as;
    for(int iz=-N+1;iz<=N-1;iz++) {
        float z=(1.0f/N)*iz;
        int first,last;
        float r=sqrt(1.0-z*z);
        for(int a=0;a<360;a+=as) {
            float angle = a*3.14159/180;
            float x=sin(angle);
            float y=cos(angle);
            ver.push_back(p3d(x*r,y*r,z));
            last = ver.size()-1;
            if(a==0)
                first = last;
            if(a>0)
                edges.push_back(std::make_pair(last-1,last));
            if(a+as >= 360)
                edges.push_back(std::make_pair(last,first));
            if(iz==-N+1) 
                edges.push_back(std::make_pair(0,last));
            else
                edges.push_back(std::make_pair(last-M,last));
            if(iz==N-1)
                edges.push_back(std::make_pair(last,1));
        }

    }
    printf("#define VERTS %d\n",int(ver.size()));
    printf("unsigned short vertices[VERTS][3] = {\n");
    for(size_t i=0;i<ver.size();i++) {
        printf("{%du,%du,%du},\n",float2half(ver[i].x),float2half(ver[i].y),float2half(ver[i].z));
    }
    printf("};\n");
    printf("#define EDGES %d\n",int(edges.size()));
    printf("unsigned edges[EDGES][2] = {\n");
    for(size_t i=0;i<edges.size();i++) {
        printf("{%d,%d},\n",edges[i].first,edges[i].second);
    }
    printf("};\n");
}
