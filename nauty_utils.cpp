/*
  This file contains code that is adapted from the C version of nauty 
  version 2.8+
*/

#include <iostream>
#include <cctype>
#include "nauty_utils.h"


// Original documentation from nauty:
/*****************************************************************************
*                                                                            *
*  put_sg(f,sg,digraph,linelength) writes the sparse graph to file f using   *
*  at most linelength characters per line.  If digraph then all directed     *
*  edges are written; else one v-w for w>=v is written.                      *
*                                                                            *
*****************************************************************************/


//static const char* const base64_str = "uvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`!@#$%^&*()-_+=,./<>?:;'{}[]|\"\\";

static const char* const base64_str = "0123456789klmnopqrstuvwxyzKLMNOPQRSTUVWXYZ!*()-_+=[]{}|<>,.?/:;'";
static const char* const trans_sp = "abcdefghij";
static const char* const trans_line = "ABCDEFGHIJ";

static size_t
compress_str(size_t label, char* buf, const char* trans) {
    size_t slen = 0;
    int r = label / 10;
    while (r > 0) {
        int q = 0;
        if (r >= 64) {
            q = r / 64;
            r = r % 64;
        }
        buf[slen] = base64_str[r];
        ++slen;
        r = q;
    }
    buf[slen] = trans[label % 10];
    ++slen;
    buf[slen] = '\0';
    return slen;
}


//static const char* const base32_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ-_+=,.";
/*
static size_t
compress(size_t num, char* buf, const char* trans)
{
    size_t label = num;
    int remainder = -1;
    if (label > 99 && label < 1000) {
        remainder = label % 100;
        if (remainder > 64) // 31) 
            remainder = -1;
        else
            label = label / 100;
    }
    int slen = itos(label,buf);
    buf[0] = trans[buf[0] - '0'];
    if (remainder > -1) {
        buf[1] = base64_str[remainder];   // base32_str[remainder];
        buf[2] = '\0';
        ++slen;
    }
    return slen;
}
*/

static size_t
compress_str_space(size_t label, char* buf) {
    return compress_str(label, buf, trans_sp);
}

static size_t
compress_str_line(size_t label, char* buf) {
    return compress_str(label, buf, trans_line);
}

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
        // slen = itos(i+labelorg,h);
        // slen = compress_str(i+labelorg,h);
        if (shorten)
            slen = compress_str_space(i+labelorg,h);
        else {
            slen = itos(i+labelorg,h);
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
            //slen = itos(e[vi+j]+labelorg,s);
            if (shorten)
                slen = compress_str_space(e[vi+j]+labelorg,s);
            else
                slen = itos(e[vi+j]+labelorg,s);

            if (!shorten) {
                if (linelength && curlen + slen + 1 >= linelength)
                {
                    graph_str.append(sep);
                    // graph_str.append(" ");
                    //putstring(f,"\n ");
                    curlen = 1;
                 }
            }

            if (!shorten)
                graph_str.append(" ");
            graph_str.append(s);
            //PUTC(' ',f);
            //putstring(f,s);
            // curlen += slen + 1;
        }
        if (shorten) {
            graph_str[graph_str.size()-1] = toupper(graph_str[graph_str.size()-1]);  // change ending space to new line
        }
        else
            graph_str.append(sep);
        // PUTC('\n',f);
    }
    return graph_str;
}

