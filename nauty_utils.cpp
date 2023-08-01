/*
  This file contains code that is adapted from the C version of nauty 
  version 2.8+
*/

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
put_sg_str(sparsegraph *sg, boolean digraph, int linelength)
{
    int *d,*e;
    int n,di;
    int i,curlen,slen;
    size_t *v,vi,j;
    char s[12];
    std::string graph_str;

    SG_VDE(sg,v,d,e);
    n = sg->nv;

    for (i = 0; i < n; ++i)
    {
        vi = v[i];
        di = d[i];
        if (di == 0) continue;
        slen = itos(i+labelorg,s);
        graph_str.append(s);
        graph_str.append(" :");
        //putstring(f,s);
        //putstring(f," :");
        curlen = slen + 2;

        for (j = 0; j < di; ++j)
        {
            if (!digraph && e[vi+j] < i) continue;
            slen = itos(e[vi+j]+labelorg,s);
            if (linelength && curlen + slen + 1 >= linelength)
            {
                graph_str.append("\n ");
                //putstring(f,"\n ");
                curlen = 2;
            }
            graph_str.append(" ");
            graph_str.append(s);
            //PUTC(' ',f);
            //putstring(f,s);
            curlen += slen + 1;
        }
        graph_str.append("\n");
        // PUTC('\n',f);
    }
    return graph_str;
}
