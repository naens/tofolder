#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "gst.h"

struct edge;

struct node
{
  int id;
  int edge_list_sz;
  struct edge **edge_list;
  struct node *sl;
  int sid_list_sz;
  int *sid_list;
  struct node *parent;
};

struct ap
{
  struct node *node;
  uint32_t ch;
  int pos;
};

struct edge
{
  int sid;
  int from;
  int to;
  struct node *end;
};

struct gst
{
  int *sa_sid_sz;		/* size of each sa[sid] */
  uint32_t **sa;
  int sid;
  int n;
  int rem;
  int nid;
  struct ap *ap;
  struct node *root;
};

/* initial size of dynamic arrays */
int DYN_SZ = 8;
/* dynamic arrays: first allocate size DYN_SZ
 * when size reaches DYN_SZ - 1, call realloc */

struct node *create_node(int id)
{
  struct node *node = (struct node *)malloc(sizeof(struct node));
  node->id = id;
  node->edge_list_sz = 0;
  /* node->edge_list = malloc(DYN_SZ * sizeof(struct edge *)); created by check_dyn*/
  node->sl = 0;
  node->sid_list_sz = 0;
  node->sid_list = (int *)malloc(DYN_SZ * sizeof(int));
  node->parent = 0;
  return node;
}

struct gst *new_gst()
{
  struct gst *gst = (struct gst *)malloc(sizeof(struct gst));
  gst->sa_sid_sz = (int *)malloc(DYN_SZ * sizeof(int));
  gst->sa_sid_sz[0] = 0;	/* first string empty */
  gst->sa = (uint32_t **)malloc(DYN_SZ * sizeof(uint32_t *));
  gst->sa[0] = (uint32_t *)malloc(DYN_SZ * sizeof(uint32_t));
  gst->sa[0][0] = 0;
  gst->sid = 0;
  gst->n = 0;
  gst->rem = 0;
  gst->root = create_node(1);
  gst->root->sl = gst->root;
  gst->ap = (struct ap *)malloc(sizeof(struct ap));
  gst->ap->node = gst->root;
  gst->ap->ch = 0;
  gst->ap->pos = 0;
  gst->nid = 1;
  return gst;
}

void free_node(struct node *node)
{
  int i;
  for (i = 0; i < node->edge_list_sz; i++)
    {
      struct edge *edge = node->edge_list[i];
      if (edge->end)
	free_node(edge->end);
      free(edge);
    }
  if (node->edge_list_sz)
    free(node->edge_list);
  free(node->sid_list);
  free(node);
}

void del_gst(struct gst *gst)
{
  int i;
  for (i = 0; i <= gst->sid; i ++)
      free(gst->sa[i]);
  free(gst->sa_sid_sz);
  free(gst->sa);
  free(gst->ap);
  gst->sid = -1;
  gst->n = 0;
  gst->rem = 0;
  free_node(gst->root);
  free(gst);
}

/* GST tree functions */
struct edge *edge_by_char(uint32_t **sa, struct node *node, uint32_t ch)
{/* find an edge in node->edge_list such as sa[edge->sid][edge->from] == ch */
  int i;
  for (i = 0; i < node->edge_list_sz; i++)
    {
      struct edge *edge = node->edge_list[i];
      if (ch == sa[edge->sid][edge->from])
	return edge;
    }
  return 0;
}

int match_ap(uint32_t **sa, struct ap *ap, uint32_t ch)
{  
  if (!ch)
    return 0;
  if (ap->pos == 0)
    return edge_by_char(sa, ap->node, ch) != 0;
  else
    {
      struct edge *aedge = edge_by_char(sa, ap->node, ap->ch);
      return ch == sa[aedge->sid][aedge->from + ap->pos];
    }
}

void add_sid(struct node *node, int sid)
{
  int has_sid = 0;
  int i;
  for (i = 0; i < node->sid_list_sz; i++)
    if (node->sid_list[i] == sid)
      {
	has_sid = 1;
	break;
      }
  if (!has_sid)
    {
      node->sid_list[node->sid_list_sz++] = sid;
      if (node->sid_list_sz % DYN_SZ == 0)
    node->sid_list = (int *)realloc(node->sid_list, (node->sid_list_sz + DYN_SZ) * sizeof(int));
    }
}

void advance_ap(uint32_t **sa, struct ap *ap, int sid, uint32_t ch)
{
  if (ap->pos == 0)
    ap->ch = ch;
  
  ap->pos++;
  struct edge *aedge = edge_by_char(sa, ap->node, ap->ch);
  if (aedge->end && aedge->to - aedge->from == ap->pos)
    {
      ap->node = aedge->end;
      add_sid(ap->node, sid);
      ap->ch = 0;
      ap->pos = 0;
    }
  
}

void follow_ap(uint32_t **sa, struct ap *ap, int sid, int from)
{
  struct edge *aedge = edge_by_char(sa, ap->node, ap->ch);
  add_sid(ap->node, sid);
  if (aedge && aedge->end != 0 && ap->pos >= aedge->to - aedge->from)
    {
      ap->node = aedge->end;
      ap->ch = sa[sid][from + aedge->to - aedge->from];
      ap->pos -= aedge->to - aedge->from;
      follow_ap(sa, ap, sid, from + aedge->to - aedge->from);
    }
}

void update_ap(struct gst *gst)
{
  if (gst->ap->node == gst->root)
    {
      if (gst->ap->pos > 0)
	{
	  gst->ap->pos--;
	  gst->ap->ch = gst->sa[gst->sid][gst->n - gst->rem + 1];
	}
      else
	gst->ap->ch = 0;
    }
  else
    {
      gst->ap->node = gst->ap->node->sl;
      struct node *node = gst->ap->node;
      while (node)
	{
	  add_sid(node, gst->sid);
	  node = node->parent;
	}
    }
  int from = (gst->rem - 1 >= gst->ap->pos) ? (gst->n - 1 - gst->ap->pos) : (gst->n - (gst->rem - 1));
  if (gst->ap->pos)
    follow_ap(gst->sa, gst->ap, gst->sid, from);
}

/* check array size so that it can include l elements 
 * if needed, realloc, increase size by sz_inc * el_sz
 * sz: number of elements already in array
 * el_sz: size of a single element of the array
 * inc_sz: number of elements that can be inserted in newly allocated space
 * l: ensure that at least l elements can be inserted 
 * l < inc_sz, the number of available cells must be <= inc_sz
 * !!! l elements must be inserted before next use */
void check_dyn(void **v, int sz, int inc_sz, int el_sz)
{
  if (sz % inc_sz == 0)
    {
      int new_n = sz + inc_sz;
      if (sz == 0)
	*v = malloc(new_n * el_sz);
      else
	*v = realloc(*v, new_n * el_sz);

      if (*v == NULL)
    {
	  exit(1);
	}
    }
}

void add_edge(struct node *node, int sid, int from, int to, struct node *end)
{
  struct edge *edge = (struct edge *)malloc(sizeof(struct edge));
  edge->sid = sid;
  edge->from = from;
  edge->to = to;
  edge->end = end;
  check_dyn((void **)&node->edge_list, node->edge_list_sz, DYN_SZ, sizeof(struct edge));
  node->edge_list[node->edge_list_sz++] = edge;
}

void new_edge(struct node *node, int sid, int from, int ch)
{
  /* add sid to sid_list if not already */
  add_sid(node, sid);

  /* new edge */
  if (ch)
    add_edge(node, sid, from, 0, 0);
}

struct node *new_node(struct node *prev, struct gst *gst, uint32_t ch)
{
  struct edge *aedge = edge_by_char(gst->sa, gst->ap->node, gst->ap->ch);

  /* new node + edges */
  struct node *new_node = create_node(++gst->nid);
  int split_point = aedge->from + gst->ap->pos;
  new_node->sl = gst->root;
  if (aedge->end != 0 || gst->sa_sid_sz[aedge->sid] - split_point != 0)
    add_edge(new_node, aedge->sid, split_point, aedge->to, aedge->end);
  if (ch)
    add_edge(new_node, gst->sid, gst->n - 1, 0, 0);
  new_node->parent = gst->ap->node;
  
  /* adding sids */
  if (aedge->end)
    for (int i = 0; i < aedge->end->sid_list_sz; i++)
      add_sid(new_node, aedge->end->sid_list[i]);
  else
    add_sid(new_node, aedge->sid);
  add_sid(new_node, gst->sid);

  /* modifying aedge */
  if (aedge->end)
    aedge->end->parent = new_node;
  aedge->to = split_point;
  aedge->end = new_node;

  if (prev)
    prev->sl = new_node;

  return new_node;
}

/* character already in sa -> insert in root tree */
void rem_loop(struct gst *gst, uint32_t ch)
{
  gst->rem++;
  gst->n++;
  struct node *prev = 0;
  while (gst->rem > 0)
    {
      if (prev && gst->rem > 1 && gst->ap->pos == 0)
	prev->sl = gst->ap->node;
      if (match_ap(gst->sa, gst->ap, ch))
	{
	  advance_ap(gst->sa, gst->ap, gst->sid, ch);
	  break;
	}
      else
	{
	  if (gst->ap->pos)
	    prev = new_node(prev, gst, ch);
	  else
	    new_edge(gst->ap->node, gst->sid, gst->n - 1, ch);
	  update_ap(gst);
	  gst->rem--;
	}
    }
}

/* Insert functions */
void add_char(struct gst *gst, uint32_t ch) /* insert character, increase sa[sid] if needed */
{
  int sid = gst->sid;

  /* in sa[sid] last char is occupied by 0 => no need to check */
  gst->sa[sid][gst->sa_sid_sz[sid]++] = ch;
  check_dyn((void **)&gst->sa[sid], gst->sa_sid_sz[sid], DYN_SZ, sizeof(uint32_t));
  gst->sa[sid][gst->sa_sid_sz[sid]] = 0;
  rem_loop(gst, ch);

  //print_strings(gst);
  //print_tree(gst);
}

void add_line_end(struct gst *gst)
{
  check_dyn((void **)&gst->sa[gst->sid], gst->sa_sid_sz[gst->sid], DYN_SZ, sizeof(uint32_t));
  rem_loop(gst, 0);
}

void line_end(struct gst *gst)	/* insert end character, create empty string */
{
  /* add line termination bytes */
  add_line_end(gst);

  /* create empty string */
  check_dyn((void **)&gst->sa, gst->sid + 1, DYN_SZ, sizeof (uint32_t *));
  check_dyn((void **)&gst->sa_sid_sz, gst->sid + 1,  DYN_SZ, sizeof (int));
  gst->sid++;

  gst->sa[gst->sid] = (uint32_t *)malloc(DYN_SZ * sizeof(uint32_t));
  gst->sa_sid_sz[gst->sid] = 0;	/* new string is empty */
  gst->sa[gst->sid][0] = 0;
  gst->n = 0;
  gst->ap->ch = 0;
  gst->ap->node = gst->root;
  gst->ap->pos = 0;

  //print_strings(gst);
  //print_tree(gst);
}

int ucs4len(uint32_t *str)
{
    int i = 0;
    while (str [i])
        i++;
    return i;
}

/* if needed: new line; allocate for string, insert characters for string */
void add_string(struct gst *gst, uint32_t *str)
{
  /* check new line: not new: do new line */
  if (gst->n)
    line_end(gst);
  
  /* realloc string */
  int len = ucs4len(str);
  gst->sa[gst->sid] = (uint32_t *)realloc(gst->sa[gst->sid], (len + 1) * sizeof(uint32_t));
  memset(gst->sa[gst->sid], 0, sizeof(uint32_t) * (len + 1));

  /* insert string */
  int i;
  for (i = 0; i < len; i++)
    {
      gst->sa[gst->sid][gst->sa_sid_sz[gst->sid]++] = str[i];
      rem_loop(gst, str[i]);
    }
  line_end(gst);
}

uint32_t *edge_string(uint32_t **sa, struct edge *edge)
{
  int to = edge->end ? edge-> to : (int)ucs4len(sa[edge->sid]);
  int edge_len = to - edge->from;
  uint32_t *edge_str = (uint32_t *)malloc (sizeof(uint32_t) * (edge_len + 1));
  memcpy(edge_str, &sa[edge->sid][edge->from], edge_len * (sizeof(uint32_t)));
  edge_str[edge_len] = 0;
  return edge_str;
}

void node_strings(uint32_t **sa, struct node *node, int *nsz, uint32_t ***nss, int str_num);

void edge_strings(uint32_t **sa, struct edge *edge, int *esz, uint32_t ***ess, int str_num)
{
  *esz = 0;
  if (edge->end == 0)
    return;

  uint32_t **nss;
  int nsz;
  node_strings(sa, edge->end, &nsz, &nss, str_num);

  uint32_t *edge_str = edge_string(sa, edge);
  if (nsz == 0 && edge->end->sid_list_sz >= str_num)
    {
      check_dyn((void **)ess, *esz, DYN_SZ, sizeof(uint32_t *));
      (*ess)[*esz] = edge_str;
      (*esz)++;
    }
  else
    {
      while (*esz < nsz)
    {
      check_dyn((void **)ess, *esz, DYN_SZ, sizeof(uint32_t *));
      uint32_t *ns = nss[*esz];
      int len1 = ucs4len(edge_str);
      int len2 = ucs4len(ns);
      (*ess)[*esz] = (uint32_t *)malloc(sizeof(uint32_t) * (len1 + len2 + 1));
      memcpy((*ess)[*esz], edge_str, len1 * sizeof(uint32_t));
      memcpy(&(*ess)[*esz][len1], ns, len2 * sizeof(uint32_t));
      (*ess)[*esz][len1 + len2] = 0;
      free(ns);
      (*esz)++;
    }
      free(edge_str);
      if (nsz)
    free(nss);
    }
}

void node_strings(uint32_t **sa, struct node *node, int *nsz, uint32_t ***nss, int str_num)
{
  *nsz = 0;
  if (node->sid_list_sz < str_num)
    return;

  int i;
  for (i = 0; i < node->edge_list_sz; i++)
    {
      struct edge *edge = node->edge_list[i];
      int esz;
      uint32_t **ess;
      edge_strings(sa, edge, &esz, &ess, str_num);
      int j;
      for (j = 0; j < esz; j++)
    {
      check_dyn((void **)nss, *nsz, DYN_SZ, sizeof(uint32_t *));
      (*nss)[*nsz] = ess[j];
      (*nsz)++;
    }
      if (esz)
    free(ess);
    }
  /* wprintf(L"NODE@%d  [3] nsz=%d\n", node->id, *nsz); */
  /* wprintf(L"NODE@%d  [3] nsz=%d ", node->id, *nsz); */
  /* for(int z = 0; z < *nsz; z++) wprintf(L" \"%S\"", (*nss)[z]); */
  /* wprintf(L"\n");	*/
}

void gst_strings(struct gst *gst, int *count, uint32_t ***strings)
{
  if (gst->sid == 1 && gst->sa_sid_sz[gst->sid] == 0)
    {
      *strings = (uint32_t **)malloc(sizeof(uint32_t *));
      (*strings)[0] = (uint32_t *)malloc(sizeof(uint32_t) * (gst->sa_sid_sz[0] + 1));
      memcpy((*strings)[0], gst->sa[0], (ucs4len(gst->sa[0]) + 1) * sizeof(uint32_t));
      *count = 1;
    }
  else
    node_strings(gst->sa, gst->root, count, strings, gst->root->sid_list_sz);
}

void longest_strings(struct gst *gst, int *count, uint32_t ***strings)
{
  int tmp_count;
  uint32_t **tmp_strings;
  gst_strings(gst, &tmp_count, &tmp_strings);
  int max_len =  0;
  int count_max = 0;
  int i;
  for (i = 0; i < tmp_count; i++)
    {
      int len = ucs4len(tmp_strings[i]);
      if (len > max_len)
	{
	  max_len = len;
	  count_max = 1;
	}
      else if (len == max_len)
	count_max++;
    }

  if (count_max)
    {
      *strings = (uint32_t **)malloc(sizeof(uint32_t *) * count_max);
      int n = 0;
      for (i = 0; i < tmp_count; i++)
    {
      int len = ucs4len(tmp_strings[i]);
	  if (len == max_len)
	    {
            (*strings)[n] = (uint32_t *)malloc(sizeof(uint32_t) * (len + 1));
            memcpy((*strings)[n++], tmp_strings[i], (len + 1) * sizeof(uint32_t));
	    }
	  free(tmp_strings[i]);	  
	}
      if (tmp_count)
	free(tmp_strings);
    }
  *count = count_max;
}
