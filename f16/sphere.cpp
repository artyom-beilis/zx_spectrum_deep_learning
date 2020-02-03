#include <vector>
#include <map>
#include <stdio.h>
#include "half.hpp"
struct p3d {
    p3d(float a=0,float b=0,float c=0) : x(a),y(b),z(c) {}
    float x,y,z;
};



#ifdef USE_F16
#define real_type "unsigned short"
#define real_format "%uu"
unsigned short to_target_format(float v)
{
    half_float::half hv(v);
    unsigned short tmp;
    memcpy(&tmp,&hv,2);
    return tmp;
}
#elif defined USE_FIXED
#define real_type "unsigned short"
#define real_format "%uu"

unsigned short to_target_format(float v)
{
    return short(v*4096+0.5);
}
#elif defined USE_FLOAT
#define real_type "float"
#define real_format "%ff"
float to_target_format(float v)
{
    return v;
}
#else
#error "Unspecified format"
#endif

struct surf {
    surf(int a,int b,int c) 
    {
        points[0]=a;
        points[1]=b;
        points[2]=c;
        edges[0]=edges[1]=edges[2]=-1;
    }
    int points[3];
    int edges[3];
};

struct edge {
    edge(int a=0,int b=0) {
        if(a<b) {
            p1=a;
            p2=b;
        }
        else {
            p2=a;
            p1=b;
        }
    }
    bool operator=(edge const &other) const
    {
        return p1==other.p1 && p2==other.p2;
    }
    bool operator<(edge const &other) const
    {
        if(p1 < other.p1)
            return true;
        if(p1 > other.p1)
            return false;
        return p2<other.p2;
    }

    int p1,p2;
};

int main()
{
    std::vector<p3d> ver;
    std::map<edge,int> edge_records;
    std::vector<std::pair<int,int> > edges;
    std::vector<surf> surfs;
    int as=60;
    int rows=180/as-1;
    int M=360/as;
    for(int r=0,ra=as;r<rows;r++,ra+=as) {
        float z=-cos(ra*3.14159/180);
        float rad=sqrt(1.0-z*z);
        for(int a=0;a<360;a+=as) {
            float angle = a*3.14159/180;
            float x=sin(angle)*rad;
            float y=cos(angle)*rad;
            ver.push_back(p3d(x,y,z));
        }
    }
    int north = ver.size();
    ver.push_back(p3d(0,0,-1.0));
    int south = ver.size();
    ver.push_back(p3d(0,0,1.0));
    for(int r=0;r<rows-1;r++) {
        for(int c=0;c<M;c++) {
            {
                int p1 = r*M+c;
                int p2 = r*M+(c+1)%M;
                int p3 = (r+1)*M+(c+1)%M;
                surfs.push_back(surf(p1,p2,p3));
            }
            {
                int p1 = (r+1)*M+(c+1)%M;
                int p2 = (r+1)*M+c;
                int p3 = r*M+c;
                surfs.push_back(surf(p1,p2,p3));
            }
        }
    }
    for(int c=0;c<M;c++) {
        {
            int p1=(c+1)%M;
            int p2=c;
            surfs.push_back(surf(p1,p2,north));
        }
        {
            int base = M*(rows-1);
            int p1=base + c;
            int p2=base + (c+1)%M;
            surfs.push_back(surf(p1,p2,south));
        }
    }
    for(unsigned i=0;i<surfs.size();i++) {
        for(int j=0;j<3;j++) {
            edge e(surfs[i].points[j],surfs[i].points[(j+1)%3]);
            auto ptr=edge_records.insert(std::make_pair(e,-1));
            if(ptr.second) {
                ptr.first->second = edges.size();
                edges.push_back(std::make_pair(e.p1,e.p2));
            }
            surfs[i].edges[j]=ptr.first->second;
        }
    }

    printf("#define VERTS %d\n",int(ver.size()));
    printf(real_type " vertices[VERTS][3] = {\n");
    for(size_t i=0;i<ver.size();i++) {
        printf("{" real_format  "," real_format "," real_format "},\n",to_target_format(ver[i].x),to_target_format(ver[i].y),to_target_format(ver[i].z));
    }
    printf("};\n");
    printf("#define EDGES %d\n",int(edges.size()));
    printf("unsigned edges[EDGES][2] = {\n");
    for(size_t i=0;i<edges.size();i++) {
        printf("{%d,%d},\n",edges[i].first,edges[i].second);
    }
    printf("};\n");
    printf("#define SURFS %d\n",int(surfs.size()));
    printf("unsigned surfs[SURFS][2][3] = {\n");
    for(size_t i=0;i<surfs.size();i++) {
        printf("{{%d,%d,%d},{%d,%d,%d}},\n",
            surfs[i].points[0],surfs[i].points[1],surfs[i].points[2],
            surfs[i].edges[0],surfs[i].edges[1],surfs[i].edges[2]);
    }
    printf("};\n");
    printf(real_type " sin_deg_a_div_5[]=");
    for(int i=0;i<=90;i+=5) {
        printf("%c" real_format,(i==0?'{':','),to_target_format(sin(i*3.14159/180)));
    }
    printf("};\n");
}
