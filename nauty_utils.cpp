/*
  This file contains code that is adapted from the C version of nauty 
  version 2.8+
*/

#include <iostream>
#include "nauty_utils.h"


// Original documentation from nauty:
/*****************************************************************************
*                                                                            *
*  put_sg(f,sg,digraph,linelength) writes the sparse graph to file f using   *
*  at most linelength characters per line.  If digraph then all directed     *
*  edges are written; else one v-w for w>=v is written.                      *
*                                                                            *
*****************************************************************************/

std::string
put_sg_str(sparsegraph *sg, const char* sep, bool shorten, bool digraph, int linelength)
{
    int *d,*e;
    int n,di;
    int curlen,slen;
    size_t *v,vi;
    char s[256];
    char h[256];
    std::string graph_str;

    SG_VDE(sg,v,d,e);
    n = sg->nv;

    // debug print
    // std::cout << "put_sg_str debug num vertices: " << n << std::endl;
    for (size_t i = 0; i < n; ++i)
    {
        vi = v[i];
        di = d[i];
        if (di == 0) continue;
        slen = itos(i+labelorg,h);
        if (!shorten) {
            graph_str.append(h);
            graph_str.append(" :");
            curlen = slen + 2;
        }

        for (size_t j = 0; j < di; ++j)
        {
            if (!digraph && e[vi+j] < i) continue;
            if (shorten && h[0] != '\0') {
                graph_str.append(h);
                curlen = slen;
                h[0] = '\0';
            }
            slen = itos(e[vi+j]+labelorg,s);
            if (linelength && curlen + slen + 1 >= linelength)
            {
                graph_str.append(sep);
                // graph_str.append(" ");
                //putstring(f,"\n ");
                curlen = 1;
            }
            graph_str.append(" ");
            graph_str.append(s);
            //PUTC(' ',f);
            //putstring(f,s);
            curlen += slen + 1;
        }
        if (!shorten || h[0] == '\0')
            graph_str.append(sep);
        // PUTC('\n',f);
    }
    return graph_str;
}
