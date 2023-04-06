/*<html><pre>  -<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="TOP">-</a>

   global.c 
   initializes all the globals of the qhull application

   see README 

   see qhull.h for qh.globals and function prototypes

   see qhull_a.h for internal functions

   copyright (c) 1993-1999, The Geometry Center
 */

#include <ivp_physics.hxx>
#include "qhull_a.hxx"

/*========= qh definition =======================*/

#if qh_QHpointer
qhT *qh_qh= NULL;	/* pointer to all global variables */
#else
qhT qh_qh;     		/* all global variables. 
			   Add "= {0}" if this causes a compiler error.  
			   Also qh_qhstat in stat.c and qhmem in mem.c.  */
#endif

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="appendprint">-</a>
  
  qh_appendprint( printFormat )
    append printFormat to qh.PRINTout unless already defined
*/
void qh_appendprint (qh_PRINT format) {
  int i;

  for (i=0; i < qh_PRINTEND; i++) {
    if (qh PRINTout[i] == format)
      break;
    if (!qh PRINTout[i]) {
      qh PRINTout[i]= format;
      break;
    }
  }
} /* appendprint */
     
/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="clock">-</a>
  
  qh_clock()
    return user CPU time in 100ths (qh_SECtick)
    only defined for qh_CLOCKtype == 2

  notes:
    use first value to determine time 0
    from Stevens '92 8.15
*/
unsigned long qh_clock (void) {

#if (qh_CLOCKtype == 2)
  struct tms time;
  static long clktck;  /* initialized first call */
  double ratio, cpu;
  unsigned long ticks;

  if (!clktck) {
    if ((clktck= sysconf (_SC_CLK_TCK)) < 0) {
      ivp_message( "qhull internal error (qh_clock): sysconf() failed.  Use qh_CLOCKtype 1 in user.h\n");
      qh_errexit (qh_ERRqhull, NULL, NULL);
    }
  }
  if (times (&time) == -1) {
    ivp_message( "qhull internal error (qh_clock): times() failed.  Use qh_CLOCKtype 1 in user.h\n");
    qh_errexit (qh_ERRqhull, NULL, NULL);
  }
  ratio= qh_SECticks / (double)clktck;
  ticks= time.tms_utime * ratio;
  return ticks; 
#else
  ivp_message( "qhull internal error (qh_clock): use qh_CLOCKtype 2 in user.h\n");
  qh_errexit (qh_ERRqhull, nullptr, nullptr); /* never returns */
  return 0;
#endif
} /* clock */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="freebuffers">-</a>
  
  qh_freebuffers()
    free up global memory buffers

  notes:
    must match qh_initbuffers()
*/
void qh_freebuffers (void) {

  trace5((qh ferr, "qh_freebuffers: freeing up global memory buffers\n"));
  /* allocated by qh_initqhull_buffers */
  qh_memfree (qh NEARzero, qh hull_dim * sizeof(realT));
  qh_memfree (qh lower_threshold, (qh input_dim+1) * sizeof(realT));
  qh_memfree (qh upper_threshold, (qh input_dim+1) * sizeof(realT));
  qh_memfree (qh lower_bound, (qh input_dim+1) * sizeof(realT));
  qh_memfree (qh upper_bound, (qh input_dim+1) * sizeof(realT));
  qh_memfree (qh gm_matrix, (qh hull_dim+1) * qh hull_dim * sizeof(coordT));
  qh_memfree (qh gm_row, (qh hull_dim+1) * sizeof(coordT *));
  qh NEARzero= qh lower_threshold= qh upper_threshold= nullptr;
  qh lower_bound= qh upper_bound= nullptr;
  qh gm_matrix= nullptr;
  qh gm_row= nullptr;
  qh_setfree (&qh other_points);
  qh_setfree (&qh del_vertices);
  qh_setfree (&qh searchset);
  if (qh line)                /* allocated by qh_readinput, freed if no error */
    P_FREE (qh line);
  if (qh half_space)
    P_FREE (qh half_space);
  if (qh temp_malloc)
    P_FREE (qh temp_malloc);
  if (qh feasible_point)      /* allocated by qh_readfeasible */
    P_FREE (qh feasible_point);
  if (qh feasible_string)     /* allocated by qh_initflags */
    P_FREE (qh feasible_string);
  qh line= qh feasible_string = nullptr;
  qh half_space= qh feasible_point= qh temp_malloc= nullptr;
  /* usually allocated by qh_readinput */
  if (qh first_point && qh POINTSmalloc) {
    P_FREE(qh first_point);
    qh first_point= nullptr;
  }
  if (qh input_points && qh input_malloc) { /* set by qh_joggleinput */
    P_FREE (qh input_points);
    qh input_points= nullptr;
  }
  trace5((qh ferr, "qh_freebuffers: finished\n"));
} /* freebuffers */


/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="freebuild">-</a>
  
  qh_freebuild( allmem )
    free global memory used by qh_initbuild and qh_buildhull
    if !allmem,
      does not free short memory (freed by qh_memfreeshort)

  design:
    free centrums
    free each vertex
    mark unattached ridges
    for each facet
      free ridges
      free outside set, coplanar set, neighbor set, ridge set, vertex set
      free facet
    free hash table
    free interior point
    free merge set
    free temporary sets
*/
void qh_freebuild (boolT allmem) {
  facetT *facet;
  vertexT *vertex;
  ridgeT *ridge, **ridgep;
  mergeT *merge, **mergep;

  trace1((qh ferr, "qh_freebuild: free memory from qh_inithull and qh_buildhull\n"));
  if (qh del_vertices)
    qh_settruncate (qh del_vertices, 0);
  if (allmem) {
    qh_clearcenters (qh_ASnone);
    while ((vertex= qh vertex_list)) {
      if (vertex->next)
        qh_delvertex (vertex);
      else {
        qh_memfree (vertex, sizeof(vertexT));
        qh newvertex_list= qh vertex_list= nullptr;
      }
    }
  }else if (qh VERTEXneighbors) {
    FORALLvertices 
      qh_setfreelong (&(vertex->neighbors));
  }
  qh VERTEXneighbors= False;
  qh GOODclosest= nullptr;
  if (allmem) {
    FORALLfacets {
      FOREACHridge_(facet->ridges)
        ridge->seen= False;
    }
    FORALLfacets {
      if (facet->visible) {
	FOREACHridge_(facet->ridges) {
	  if (!otherfacet_(ridge, facet)->visible)
	    ridge->seen= True;  /* an unattached ridge */
	}
      }
    }
    while ((facet= qh facet_list)) {
      FOREACHridge_(facet->ridges) {
        if (ridge->seen) {
          qh_setfree(&(ridge->vertices));
          qh_memfree(ridge, sizeof(ridgeT));
        }else
          ridge->seen= True;
      }
      qh_setfree (&(facet->outsideset));
      qh_setfree (&(facet->coplanarset));
      qh_setfree (&(facet->neighbors));
      qh_setfree (&(facet->ridges));
      qh_setfree (&(facet->vertices));
      if (facet->next)
        qh_delfacet (facet);
      else {
        qh_memfree (facet, sizeof(facetT));
        qh visible_list= qh newfacet_list= qh facet_list= nullptr;
      }
    }
  }else {
    FORALLfacets {
      qh_setfreelong (&(facet->outsideset));
      qh_setfreelong (&(facet->coplanarset));
      if (!facet->simplicial) {
        qh_setfreelong (&(facet->neighbors));
        qh_setfreelong (&(facet->ridges));
        qh_setfreelong (&(facet->vertices));
      }
    }
  }
  qh_setfree (&(qh hash_table));
  qh_memfree (qh interior_point, qh normal_size);
  qh interior_point= nullptr;
  FOREACHmerge_(qh facet_mergeset)  /* usually empty */
    qh_memfree (merge, sizeof(mergeT));
  qh facet_mergeset= nullptr;  /* temp set */
  qh degen_mergeset= nullptr;  /* temp set */
  qh_settempfree_all();
} /* freebuild */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="freeqhull">-</a>
  
  qh_freeqhull( allmem )
    free global memory
    if !allmem,
      does not free short memory (freed by qh_memfreeshort)

  notes:
    sets qh.NOerrexit in case caller forgets to

  design:
    free global and temporary memory from qh_initbuild and qh_buildhull
    free buffers
    free statistics
*/
void qh_freeqhull (boolT allmem) {

  trace1((qh ferr, "qh_freeqhull: free global memory\n"));
  qh NOerrexit= True;  /* no more setjmp since called at exit */
  qh_freebuild (allmem);
  qh_freebuffers();
  qh_freestatistics();
#if qh_QHpointer
  P_FREE (qh_qh);
  qh_qh= NULL;
#else
  memset((char *)&qh_qh, 0, sizeof(qhT));
  qh NOerrexit= True;
#endif
} /* freeqhull */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="init_A">-</a>
  
  qh_init_A( infile, outfile, errfile, argc, argv )
    initialize memory and stdio files
    convert input options to option string (qh.qhull_command)

  notes:
    infile may be NULL if qh_readpoints() is not called
    
    errfile should always be defined.  It is used for reporting
    errors.  outfile is used for output and format options.
    
    argc/argv may be 0/NULL
    
    called before error handling initialized
    qh_errexit() may not be used
*/
void qh_init_A (FILE *infile, FILE *outfile, FILE *errfile, int argc, char *argv[]) {
  qh_meminit (errfile);
  qh_initqhull_start (infile, outfile, errfile); 
  qh_init_qhull_command (argc, argv);
} /* init_A */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="init_B">-</a>
  
  qh_init_B( points, numpoints, dim, ismalloc )
    initialize globals for points array
    
    points has numpoints dim-dimensional points
      points[0] is the first coordinate of the first point
      points[1] is the second coordinate of the first point
      points[dim] is the first coordinate of the second point
    
    ismalloc=True
      Qhull will call free(points) on exit or input transformation
    ismalloc=False
      Qhull will allocate a new point array if needed for input transformation
    
    qh.qhull_command 
      is the option string.  
      It is defined by qh_init_B(), qh_qhull_command(), or qh_initflags

  returns:
    if qh.PROJECTinput or (qh.DELAUNAY and qh.PROJECTdelaunay)
      projects the input to a new point array
 
        if qh.DELAUNAY, 
          qh.hull_dim is increased by one
        if qh.ATinfinity, 
          qh_projectinput adds point-at-infinity for Delaunay tri.
 
    if qh.SCALEinput
      changes the upper and lower bounds of the input, see qh_scaleinput()
 
    if qh.ROTATEinput
      rotates the input by a random rotation, see qh_rotateinput()
      if qh.DELAUNAY
        rotates about the last coordinate
        
  notes:
    called after points are defined
    qh_errexit() may be used
*/
void qh_init_B (coordT *points, int numpoints, int dim, boolT ismalloc) { 
  qh_initqhull_globals (points, numpoints, dim, ismalloc);
  if (qhmem.LASTsize == 0)
    qh_initqhull_mem();
  /* mem.c and qset.c are initialized */
  qh_initqhull_buffers();
  qh_initthresholds (qh qhull_command);
  if (qh PROJECTinput || (qh DELAUNAY && qh PROJECTdelaunay)) 
    qh_projectinput();
  if (qh SCALEinput)
    qh_scaleinput();
  if (qh ROTATErandom >= 0) {
    qh_randommatrix (qh gm_matrix, qh hull_dim, qh gm_row);
    if (qh DELAUNAY) {
      int k, lastk= qh hull_dim-1;
      for (k= 0; k < lastk; k++) {
        qh gm_row[k][lastk]= 0.0;
        qh gm_row[lastk][k]= 0.0;
      }
      qh gm_row[lastk][lastk]= 1.0;
    }
    qh_gram_schmidt (qh hull_dim, qh gm_row);
    qh_rotateinput (qh gm_row);
  }
} /* init_B */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="init_qhull_command">-</a>
  
  qh_init_qhull_command( argc, argv )
    build qh.qhull_command from argc/argv

  returns:
    a space-deliminated string of options (just as typed)
    
  notes:
    makes option string easy to input and output 
    
    argc/argv may be 0/NULL
*/
void qh_init_qhull_command(int argc, char *argv[]) {
  int i;
  char *s;

  if (argc) {
    if ((s= strrchr( argv[0], '\\'))) /* Borland gives full path */
      strcpy (qh qhull_command, s+1);
    else
      strcpy (qh qhull_command, argv[0]);
    if ((s= strstr (qh qhull_command, ".EXE"))
    ||  (s= strstr (qh qhull_command, ".exe")))
      *s= '\0';
  }
  for (i=1; i < argc; i++) {
    if (strlen (qh qhull_command) + strlen(argv[i]) + 1 < sizeof(qh qhull_command)) {
      strcat (qh qhull_command, " ");
      strcat (qh qhull_command, argv[i]);
    }else {
      ivp_message( "qhull input error: more than %d characters in command line\n",
        (int)sizeof(qh qhull_command));
      exit (1);  /* can not use qh_errexit */
    }
  }
} /* init_qhull_command */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initflags">-</a>
  
  qh_initflags( commandStr )
    set flags and initialized constants from commandStr

  returns:
    sets qh.qhull_command to command if needed
  
  notes:
    ignores first word (e.g., "qhull d")
    use qh_strtol/strtod since strtol/strtod may or may not skip trailing spaces
  
  see:
    qh_initthresholds() continues processing of 'Pdn' and 'PDn'
    'prompt' in unix.c for documentation

  design:
    for each space-deliminated option group
      if top-level option
        check syntax
        append approriate option to option string
        set appropriate global variable or append printFormat to print options
      else
        for each sub-option
          check syntax
          append approriate option to option string
          set appropriate global variable or append printFormat to print options
      
    
*/
void qh_initflags(char *command) {
  int k, i, lastproject;
  char *s= command, *t, *prev_s, *start, key;
  boolT isgeom= False, wasproject;
  realT r;

  if (command != &qh qhull_command[0]) {
    *qh qhull_command= '\0';
    strncat( qh qhull_command, command, sizeof( qh qhull_command));
  }
  while (*s && !isspace(*s))  /* skip program name */
    s++;
  while (*s) {
    while (*s && isspace(*s))
      s++;
    if (*s == '-')
      s++;
    if (!*s)
      break;
    prev_s= s;
    switch (*s++) {
    case 'd':
      qh_option ("delaunay", nullptr, nullptr);
      qh DELAUNAY= True;
      break;
    case 'f':
      qh_option ("facets", nullptr, nullptr);
      qh_appendprint (qh_PRINTfacets);
      break;
    case 'i':
      qh_option ("incidence", nullptr, nullptr);
      qh_appendprint (qh_PRINTincidences);
      break;
    case 'm':
      qh_option ("mathematica", nullptr, nullptr);
      qh_appendprint (qh_PRINTmathematica);
      break;
    case 'n':
      qh_option ("normals", nullptr, nullptr);
      qh_appendprint (qh_PRINTnormals);
      break;
    case 'o':
      qh_option ("offFile", nullptr, nullptr);
      qh_appendprint (qh_PRINToff);
      break;
    case 'p':
      qh_option ("points", nullptr, nullptr);
      qh_appendprint (qh_PRINTpoints);
      break;
    case 's':
      qh_option ("summary", nullptr, nullptr);
      qh PRINTsummary= True;
      break;
    case 'v':
      qh_option ("voronoi", nullptr, nullptr);
      qh VORONOI= True;
      qh DELAUNAY= True;
      break;
    case 'A':
      if (!isdigit(*s) && *s != '.' && *s != '-') 
	fprintf(qh ferr, "qhull warning: no maximum cosine angle given for option 'An'.  Ignored.\n");
      else {
	if (*s == '-') {
	  qh premerge_cos= -qh_strtod (s, &s);
          qh_option ("Angle-premerge-", nullptr, &qh premerge_cos);
	  qh PREmerge= True;
	}else {
	  qh postmerge_cos= qh_strtod (s, &s);
          qh_option ("Angle-postmerge", nullptr, &qh postmerge_cos);
	  qh POSTmerge= True;
	}
	qh MERGING= True; 
      }
      break;
    case 'C':
      if (!isdigit(*s) && *s != '.' && *s != '-')
	fprintf(qh ferr, "qhull warning: no centrum radius given for option 'Cn'.  Ignored.\n");
      else {
	if (*s == '-') {
	  qh premerge_centrum= -qh_strtod (s, &s);
          qh_option ("Centrum-premerge-", nullptr, &qh premerge_centrum);
	  qh PREmerge= True;
	}else {
	  qh postmerge_centrum= qh_strtod (s, &s);
          qh_option ("Centrum-postmerge", nullptr, &qh postmerge_centrum);
	  qh POSTmerge= True;
	}
	qh MERGING= True; 
      }
      break;
    case 'E':
      if (*s == '-')
	fprintf(qh ferr, "qhull warning: negative maximum roundoff given for option 'An'.  Ignored.\n");
      else if (!isdigit(*s))
	fprintf(qh ferr, "qhull warning: no maximum roundoff given for option 'En'.  Ignored.\n");
      else {
	qh DISTround= qh_strtod (s, &s);
        qh_option ("Distance-roundoff", nullptr, &qh DISTround);
	qh SETroundoff= True;
      }
      break;
    case 'H':
      start= s;
      qh HALFspace= True;
      qh_strtod (s, &t);
      while (t > s)  {
        if (*t && !isspace (*t)) {
	  if (*t == ',')
	    t++;
	  else
	    ivp_message( "qhull warning: origin for Halfspace intersection should be 'Hn,n,n,...'\n");
	}
        s= t;
	qh_strtod (s, &t);
      }
      if (start < t) {
        if (!(qh feasible_string= (char*)p_calloc (t-start+1, 1))) {
          fprintf(qh ferr, "qhull error: insufficient memory for 'Hn,n,n'\n");
          qh_errexit(qh_ERRmem, nullptr, nullptr);
        }
        strncpy (qh feasible_string, start, t-start);
        qh_option ("Halfspace-about", nullptr, nullptr);
        qh_option (qh feasible_string, nullptr, nullptr);
      }else
        qh_option ("Halfspace", nullptr, nullptr);
      break;
    case 'R':
      if (!isdigit(*s))
	fprintf(qh ferr, "qhull warning: missing random perturbation for option 'Rn'.  Ignored\n");
      else {
	qh RANDOMfactor= qh_strtod (s, &s);
        qh_option ("Random_perturb", nullptr, &qh RANDOMfactor);
        qh RANDOMdist= True;
      }
      break;
    case 'V':
      if (!isdigit(*s) && *s != '-')
	fprintf(qh ferr, "qhull warning: missing visible distance for option 'Vn'.  Ignored\n");
      else {
	qh MINvisible= qh_strtod (s, &s);
        qh_option ("Visible", nullptr, &qh MINvisible);
      }
      break;
    case 'U':
      if (!isdigit(*s) && *s != '-')
	fprintf(qh ferr, "qhull warning: missing coplanar distance for option 'Un'.  Ignored\n");
      else {
	qh MAXcoplanar= qh_strtod (s, &s);
        qh_option ("U-coplanar", nullptr, &qh MAXcoplanar);
      }
      break;
    case 'W':
      if (*s == '-')
	fprintf(qh ferr, "qhull warning: negative outside width for option 'Wn'.  Ignored.\n");
      else if (!isdigit(*s))
	fprintf(qh ferr, "qhull warning: missing outside width for option 'Wn'.  Ignored\n");
      else {
	qh MINoutside= qh_strtod (s, &s);
        qh_option ("W-outside", nullptr, &qh MINoutside);
        qh APPROXhull= True;
      }
      break;
    /************  sub menus ***************/
    case 'F':
      while (*s && !isspace(*s)) {
	switch(*s++) {
	case 'a':
	  qh_option ("Farea", nullptr, nullptr);
	  qh_appendprint (qh_PRINTarea);
	  qh GETarea= True;
	  break;
	case 'A':
	  qh_option ("FArea-total", nullptr, nullptr);
	  qh GETarea= True;
	  break;
        case 'c':
          qh_option ("Fcoplanars", nullptr, nullptr);
          qh_appendprint (qh_PRINTcoplanars);
          break;
        case 'C':
          qh_option ("FCentrums", nullptr, nullptr);
          qh_appendprint (qh_PRINTcentrums);
          break;
	case 'd':
          qh_option ("Fd-cdd-in", nullptr, nullptr);
	  qh CDDinput= True;
	  break;
	case 'D':
          qh_option ("FD-cdd-out", nullptr, nullptr);
	  qh CDDoutput= True;
	  break;
	case 'F':
	  qh_option ("FFacets-xridge", nullptr, nullptr);
          qh_appendprint (qh_PRINTfacets_xridge);
	  break;
        case 'i':
          qh_option ("Finner", nullptr, nullptr);
          qh_appendprint (qh_PRINTinner);
          break;
        case 'I':
          qh_option ("FIds", nullptr, nullptr);
          qh_appendprint (qh_PRINTids);
          break;
        case 'm':
          qh_option ("Fmerges", nullptr, nullptr);
          qh_appendprint (qh_PRINTmerges);
          break;
        case 'n':
          qh_option ("Fneighbors", nullptr, nullptr);
          qh_appendprint (qh_PRINTneighbors);
          break;
        case 'N':
          qh_option ("FNeighbors-vertex", nullptr, nullptr);
          qh_appendprint (qh_PRINTvneighbors);
          break;
        case 'o':
          qh_option ("Fouter", nullptr, nullptr);
          qh_appendprint (qh_PRINTouter);
          break;
	case 'O':
	  if (qh PRINToptions1st) {
	    qh_option ("FOptions", nullptr, nullptr);
	    qh_appendprint (qh_PRINToptions);
	  }else 
	    qh PRINToptions1st= True;
	  break;
	case 'p':
	  qh_option ("Fpoint-intersect", nullptr, nullptr);
	  qh_appendprint (qh_PRINTpointintersect);
	  break;
	case 'P':
	  qh_option ("FPoint-nearest", nullptr, nullptr);
	  qh_appendprint (qh_PRINTpointnearest);
	  break;
	case 'Q':
	  qh_option ("FQhull", nullptr, nullptr);
	  qh_appendprint (qh_PRINTqhull);
	  break;
        case 's':
          qh_option ("Fsummary", nullptr, nullptr);
          qh_appendprint (qh_PRINTsummary);
          break;
        case 'S':
          qh_option ("FSize", nullptr, nullptr);
          qh_appendprint (qh_PRINTsize);
          qh GETarea= True;
          break;
        case 't':
          qh_option ("Ftriangles", nullptr, nullptr);
          qh_appendprint (qh_PRINTtriangles);
          break;
        case 'v':
          /* option set in qh_initqhull_globals */
          qh_appendprint (qh_PRINTvertices);
          break;
        case 'V':
          qh_option ("FVertex-average", nullptr, nullptr);
          qh_appendprint (qh_PRINTaverage);
          break;
	case 'x':
	  qh_option ("Fxtremes", nullptr, nullptr);
	  qh_appendprint (qh_PRINTextremes);
	  break;
	default:
	  s--;
	  ivp_message( "qhull warning: unknown 'F' output option %c, rest ignored\n", (int)s[0]);
	  while (*++s && !isspace(*s)){;};
	  break;
	}
      }
      break;
    case 'G':
      isgeom= True;
      qh_appendprint (qh_PRINTgeom);
      while (*s && !isspace(*s)) {
	switch(*s++) {
        case 'a':
          qh_option ("Gall-points", nullptr, nullptr);
          qh PRINTdots= True;
          break;
        case 'c':
          qh_option ("Gcentrums", nullptr, nullptr);
          qh PRINTcentrums= True;
          break;
	case 'h':
          qh_option ("Gintersections", nullptr, nullptr);
	  qh DOintersections= True;
	  break;
	case 'i':
          qh_option ("Ginner", nullptr, nullptr);
	  qh PRINTinner= True;
	  break;
	case 'n':
          qh_option ("Gno-planes", nullptr, nullptr);
	  qh PRINTnoplanes= True;
	  break;
	case 'o':
          qh_option ("Gouter", nullptr, nullptr);
	  qh PRINTouter= True;
	  break;
	case 'p':
          qh_option ("Gpoints", nullptr, nullptr);
	  qh PRINTcoplanar= True;
	  break;
	case 'r':
          qh_option ("Gridges", nullptr, nullptr);
	  qh PRINTridges= True;
	  break;
	case 't':
          qh_option ("Gtransparent", nullptr, nullptr);
	  qh PRINTtransparent= True;
	  break;
	case 'v':
          qh_option ("Gvertices", nullptr, nullptr);
	  qh PRINTspheres= True;
	  break;
	case 'D':
	  if (!isdigit (*s))
	    ivp_message( "qhull input error: missing dimension for option 'GDn'\n");
	  else {
	    if (qh DROPdim >= 0)
	      ivp_message( "qhull warning: can only drop one dimension.  Previous 'GD%d' ignored\n",
	           qh DROPdim);
  	    qh DROPdim= qh_strtol (s, &s);
            qh_option ("GDrop-dim", &qh DROPdim, nullptr);
          }
	  break;
	default:
	  s--;
	  ivp_message( "qhull warning: unknown 'G' print option %c, rest ignored\n", (int)s[0]);
	  while (*++s && !isspace(*s)){;};
	  break;
	}
      }
      break;
    case 'P':
      while (*s && !isspace(*s)) {
	switch(*s++) {
	case 'd': case 'D':  /* see qh_initthresholds() */
	  key= s[-1];
	  i= qh_strtol (s, &s);
	  r= 0;
	  if (*s == ':') {
	    s++;
	    r= qh_strtod (s, &s);
	  }
	  if (key == 'd')
  	    qh_option ("Pdrop-facets-dim-less", &i, &r);
  	  else
  	    qh_option ("PDrop-facets-dim-more", &i, &r);
	  break;
        case 'g':
          qh_option ("Pgood-facets", nullptr, nullptr);
          qh PRINTgood= True;
          break;
        case 'G':
          qh_option ("PGood-facet-neighbors", nullptr, nullptr);
          qh PRINTneighbors= True;
          break;
        case 'o':
          qh_option ("Poutput-forced", nullptr, nullptr);
          qh FORCEoutput= True;
          break;
        case 'p':
          qh_option ("Pprecision-ignore", nullptr, nullptr);
          qh PRINTprecision= False;
          break;
	case 'A':
	  if (!isdigit (*s))
	    ivp_message( "qhull input error: missing facet count for keep area option 'PAn'\n");
	  else {
  	    qh KEEParea= qh_strtol (s, &s);
            qh_option ("PArea-keep", &qh KEEParea, nullptr);
            qh GETarea= True;
          }
	  break;
	case 'F':
	  if (!isdigit (*s))
	    ivp_message( "qhull input error: missing facet area for option 'PFn'\n");
	  else {
  	    qh KEEPminArea= qh_strtod (s, &s);
            qh_option ("PFacet-area-keep", nullptr, &qh KEEPminArea);
            qh GETarea= True;
          }
	  break;
	case 'M':
	  if (!isdigit (*s))
	    ivp_message( "qhull input error: missing merge count for option 'PMn'\n");
	  else {
  	    qh KEEPmerge= qh_strtol (s, &s);
            qh_option ("PMerge-keep", &qh KEEPmerge, nullptr);
          }
	  break;
	default:
	  s--;
	  ivp_message( "qhull warning: unknown 'P' print option %c, rest ignored\n", (int)s[0]);
	  while (*++s && !isspace(*s));
	  break;
	}
      }
      break;
    case 'Q':
      lastproject= -1;
      while (*s && !isspace(*s)) {
	switch(*s++) {
	case 'b': case 'B':  /* handled by qh_initthresholds */
	  key= s[-1];
	  if (key == 'b' && *s == 'B') {
	    s++;
	    r= qh_DEFAULTbox;
	    qh SCALEinput= True;
	    qh_option ("QbBound-unit-box", nullptr, &r);
	    break;
	  }
	  if (key == 'b' && *s == 'b') {
	    s++;
	    qh SCALElast= True;
	    qh_option ("Qbbound-last", nullptr, nullptr);
	    break;
	  }
	  k= qh_strtol (s, &s);
	  r= 0.0;
	  wasproject= False;
	  if (*s == ':') {
	    s++;
	    if ((r= qh_strtod(s, &s)) == 0.0) {
 	      t= s;            /* need true dimension for memory allocation */
	      while (*t && !isspace(*t)) {
	        if (toupper(*t++) == 'B' 
	         && k == qh_strtol (t, &t)
	         && *t++ == ':'
	         && qh_strtod(t, &t) == 0.0) {
	          qh PROJECTinput++;
	          trace2((qh ferr, "qh_initflags: project dimension %d\n", k));
	          qh_option ("Qb-project-dim", &k, nullptr);
		  wasproject= True;
	          lastproject= k;
	          break;
		}
	      }
	    }
  	  }
	  if (!wasproject) {
	    if (lastproject == k && r == 0.0) 
	      lastproject= -1;  /* doesn't catch all possible sequences */
	    else if (key == 'b') {
	      qh SCALEinput= True;
	      if (r == 0.0)
		r= -qh_DEFAULTbox;
	      qh_option ("Qbound-dim-low", &k, &r);
	    }else {
	      qh SCALEinput= True;
	      if (r == 0.0)
		r= qh_DEFAULTbox;
	      qh_option ("QBound-dim-high", &k, &r);
	    }
	  }
	  break;
	case 'c':
	  qh_option ("Qcoplanar-keep", nullptr, nullptr);
	  qh KEEPcoplanar= True;
	  break;
	case 'f':
	  qh_option ("Qfurthest-outside", nullptr, nullptr);
	  qh BESToutside= True;
	  break;
	case 'g':
	  qh_option ("Qgood-facets-only", nullptr, nullptr);
	  qh ONLYgood= True;
	  break;
	case 'i':
	  qh_option ("Qinterior-keep", nullptr, nullptr);
	  qh KEEPinside= True;
	  break;
	case 'm':
	  qh_option ("Qmax-outside-only", nullptr, nullptr);
	  qh ONLYmax= True;
	  break;
	case 'r':
	  qh_option ("Qrandom-outside", nullptr, nullptr);
	  qh RANDOMoutside= True;
	  break;
	case 's':
	  qh_option ("Qsearch-initial-simplex", nullptr, nullptr);
	  qh ALLpoints= True;
	  break;
	case 'u':
	  qh_option ("QupperDelaunay", nullptr, nullptr);
	  qh UPPERdelaunay= True;
	  break;
	case 'v':
	  qh_option ("Qvertex-neighbors-convex", nullptr, nullptr);
	  qh TESTvneighbors= True;
	  break;
	case 'x':
	  qh_option ("Qxact-merge", nullptr, nullptr);
	  qh MERGEexact= True;
	  qh MERGING= True;
	  break;
	case 'z':
	  qh_option ("Qz-infinity-point", nullptr, nullptr);
	  qh ATinfinity= True;
	  break;
	case '0':
	  qh_option ("Q0-no-premerge", nullptr, nullptr);
	  qh NOpremerge= True;
	  break; 
	case '1':
	  qh_option ("Q1-no-angle-sort", nullptr, nullptr);
	  qh ANGLEmerge= False;
	  goto LABELcheckdigit;
	  break; /* no warnings */
	case '2':
	  qh_option ("Q2-no-merge-independent", nullptr, nullptr);
	  qh MERGEindependent= False;
	  goto LABELcheckdigit;
	  break; /* no warnings */
	case '3':
	  qh_option ("Q3-no-merge-vertices", nullptr, nullptr);
	  qh MERGEvertices= False;
	LABELcheckdigit:
	  if (isdigit(*s)) 
	    ivp_message( "qhull warning: can not follow '1', '2', or '3' with a digit.  '%c' skipped.\n",
	             *s++);
	  break;
	case '4':
	  qh_option ("Q4-avoid-old-into-new", nullptr, nullptr);
	  qh AVOIDold= True;
	  break;
	case '5':
	  qh_option ("Q5-no-check-outer", nullptr, nullptr);
	  qh SKIPcheckmax= True;
	  break;
	case '6':
	  qh_option ("Q6-no-concave-merge", nullptr, nullptr);
	  qh SKIPconvex= True;
	  break;
	case '7':
	  qh_option ("Q7-no-breadth-first", nullptr, nullptr);
	  qh VIRTUALmemory= True;
	  break;
	case '8':
	  qh_option ("Q8-no-near-inside", nullptr, nullptr);
	  qh NOnearinside= True;
	  break;
	case '9':
	  qh_option ("Q9-pick-furthest", nullptr, nullptr);
	  qh PICKfurthest= True;
	  break;
	case 'G':
	  i= qh_strtol (s, &t);
	  if (qh GOODpoint) 
	    ivp_message( "qhull warning: good point already defined for option 'QGn'.  Ignored\n");
          else if (s == t)
	    ivp_message( "qhull warning: missing good point id for option 'QGn'.  Ignored\n");
	  else if (i < 0 || *s == '-') { 
 	    qh GOODpoint= i-1;
  	    qh_option ("QGood-if-dont-see-point", &i, nullptr);
	  }else {
 	    qh GOODpoint= i+1;
  	    qh_option ("QGood-if-see-point", &i, nullptr);
  	  }
 	  s= t;
	  break;
	case 'J':
          if (!isdigit(*s) && *s != '-')
   	    qh JOGGLEmax= 0.0;
	  else {
 	    qh JOGGLEmax= (realT) qh_strtod (s, &s);
            qh_option ("QJoggle", nullptr, &qh JOGGLEmax);
	  }
	  break;
	case 'R':
          if (!isdigit(*s) && *s != '-')
	    ivp_message( "qhull warning: missing random seed for option 'QRn'.  Ignored\n");
	  else {
 	    qh ROTATErandom= i= qh_strtol(s, &s);
   	    if (i > 0)
   	      qh_option ("QRotate-id", &i, nullptr);
	    else if (i < -1)
   	      qh_option ("QRandom-seed", &i, nullptr);
          }
	  break;
	case 'V':
	  i= qh_strtol (s, &t);
	  if (qh GOODvertex) 
	    ivp_message( "qhull warning: good vertex already defined for option 'QVn'.  Ignored\n");
          else if (s == t)
	    ivp_message( "qhull warning: no good point id given for option 'QVn'.  Ignored\n");
	  else if (i < 0) {
 	    qh GOODvertex= i - 1;
 	    qh_option ("QV-good-facets-not-point", &i, nullptr);
	  }else {
  	    qh_option ("QV-good-facets-point", &i, nullptr);
	    qh GOODvertex= i + 1;
          }
 	  s= t;
	  break;
	default:
	  s--;
	  ivp_message( "qhull warning: unknown 'Q' qhull option %c, rest ignored\n", (int)s[0]);
	  while (*++s && !isspace(*s));
	  break;
	}
      }
      break;
    case 'T':
      while (*s && !isspace(*s)) {
	if (isdigit(*s) || *s == '-')
	  qh IStracing= qh_strtol(s, &s);
	else switch(*s++) {
	case 'c':
          qh_option ("Tcheck-frequently", nullptr, nullptr);
       	  qh CHECKfrequently= True;
	  break;
	case 's':
          qh_option ("Tstatistics", nullptr, nullptr);
	  qh PRINTstatistics= True;
	  break;
	case 'v':
          qh_option ("Tverify", nullptr, nullptr);
	  qh VERIFYoutput= True;
	  break;
	case 'z':
	  if (!qh fout)
	    ivp_message( "qhull warning: output file undefined (stdout).  Option 'Tz' ignored.\n");
	  else {
	    qh_option ("Tz-stdout", nullptr, nullptr);
  	    qh ferr= qh fout;
  	    qhmem.ferr= qh fout;
	  }
	  break;
	case 'C':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing point id for cone for trace option 'TCn'.  Ignored\n");
	  else {
	    i= qh_strtol (s, &s);
	    qh_option ("TCone-stop", &i, nullptr);
	    qh STOPcone= i + 1;
          }
	  break;
	case 'F':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing frequency count for trace option 'TFn'.  Ignored\n");
	  else {
	    qh REPORTfreq= qh_strtol (s, &s);
            qh_option ("TFacet-log", &qh REPORTfreq, nullptr);
	    qh REPORTfreq2= qh REPORTfreq/2;  /* for tracemerging() */
	  }
	  break;
	case 'O':
	  if (s[0] != ' ' || s[1] == '\"' || isspace (s[1])) {
	    s++;
	    ivp_message( "qhull warning: option 'TO' mistyped.\nUse 'TO', one space, file name, and space or end-of-line.\nThe file name may be enclosed in single quotes.\nDo not use double quotes.  Option 'FO' ignored.\n");
	  }else {  /* not a procedure because of qh_option (filename, NULL, NULL); */
	    char filename[500], *t= filename;
	    boolT isquote= False;

	    s++;
	    if (*s == '\'') {
	      isquote= True;
	      s++;
	    }
	    while (*s) {
	      if (t - filename >= (int)(sizeof (filename)-2)) {
		ivp_message( "qhull error: filename for 'TO' too long.\n");
		qh_errexit (qh_ERRinput, nullptr, nullptr);
	      }
	      if (isquote) {
		if (*s == '\'') {
		  s++;
		  isquote= False;
		  break;
		}
	      }else if (isspace (*s))
		break;
	      *(t++)= *s++;
	    }
	    *t= '\0';
#if defined(PSXII) || defined(GEKKO)
#else
	    if (isquote) 
	      ivp_message( "qhull error: missing end quote for option 'TO'.  Rest of line ignored.\n");
	    else if (!freopen (filename, "w", nullptr /*stdout*/)) {
	      ivp_message( "qhull error: could not open file \"%s\".", filename);
	      qh_errexit (qh_ERRinput, nullptr, nullptr);
	    }else {
	      qh_option ("TOutput-file", nullptr, nullptr);
	      qh_option (filename, nullptr, nullptr);
	    }
#endif
	  }
	  break;
	case 'P':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing point id for trace option 'TPn'.  Ignored\n");
	  else {
	    qh TRACEpoint= qh_strtol (s, &s);
            qh_option ("Trace-point", &qh TRACEpoint, nullptr);
          }
	  break;
	case 'M':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing merge id for trace option 'TMn'.  Ignored\n");
	  else {
	    qh TRACEmerge= qh_strtol (s, &s);
            qh_option ("Trace-merge", &qh TRACEmerge, nullptr);
          }
	  break;
	case 'R':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing rerun count for trace option 'TRn'.  Ignored\n");
	  else {
	    qh RERUN= qh_strtol (s, &s);
            qh_option ("TRerun", &qh RERUN, nullptr);
          }
	  break;
	case 'V':
	  i= qh_strtol (s, &t);
	  if (s == t)
	    ivp_message( "qhull warning: missing furthest point id for trace option 'TVn'.  Ignored\n");
	  else if (i < 0) {
	    qh STOPpoint= i - 1;
            qh_option ("TV-stop-before-point", &i, nullptr);
	  }else {
	    qh STOPpoint= i + 1;
            qh_option ("TV-stop-after-point", &i, nullptr);
          }
          s= t;
	  break;
	case 'W':
	  if (!isdigit(*s))
	    ivp_message( "qhull warning: missing max width for trace option 'TWn'.  Ignored\n");
	  else {
 	    qh TRACEdist= (realT) qh_strtod (s, &s);
            qh_option ("TWide-trace", nullptr, &qh TRACEdist);
          }
	  break;
	default:
	  s--;
	  ivp_message( "qhull warning: unknown 'T' trace option %c, rest ignored\n", (int)s[0]);
	  while (*++s && !isspace(*s));
	  break;
	}
      }
      break;
    default:
      ivp_message( "qhull warning: unknown flag %c (%x)\n", (int)s[-1],
	       (int)s[-1]);
      break;
    }
    if (s-1 == prev_s && *s && !isspace(*s)) {
      ivp_message( "qhull warning: missing space after flag %c (%x); reserved for menu. Skipped.\n",
	       (int)*prev_s, (int)*prev_s);
      while (*s && !isspace(*s))
	s++;
    }
  }
  if (isgeom && !qh FORCEoutput && qh PRINTout[1])
    ivp_message( "qhull warning: additional output formats are not compatible with Geomview\n");
  /* set derived values in qh_initqhull_globals */
} /* initflags */


/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initqhull_buffers">-</a>
  
  qh_initqhull_buffers()
    initialize global memory buffers

  notes:
    must match qh_freebuffers()
*/
void qh_initqhull_buffers (void) {
  int k;

  qh TEMPsize= (qhmem.LASTsize - sizeof (setT))/SETelemsize;
  if (qh TEMPsize <= 0 || qh TEMPsize > qhmem.LASTsize)
    qh TEMPsize= 8;  /* e.g., if qh_NOmem */
  qh other_points= qh_setnew (qh TEMPsize);
  qh del_vertices= qh_setnew (qh TEMPsize);
  qh searchset= qh_setnew (qh TEMPsize);
  qh NEARzero= (realT *)qh_memalloc(qh hull_dim * sizeof(realT));
  qh lower_threshold= (realT *)qh_memalloc((qh input_dim+1) * sizeof(realT));
  qh upper_threshold= (realT *)qh_memalloc((qh input_dim+1) * sizeof(realT));
  qh lower_bound= (realT *)qh_memalloc((qh input_dim+1) * sizeof(realT));
  qh upper_bound= (realT *)qh_memalloc((qh input_dim+1) * sizeof(realT));
  for(k= qh input_dim+1; k--; ) {
    qh lower_threshold[k]= -REALmax;
    qh upper_threshold[k]= REALmax;
    qh lower_bound[k]= -REALmax;
    qh upper_bound[k]= REALmax;
  }
  qh gm_matrix= (coordT *)qh_memalloc((qh hull_dim+1) * qh hull_dim * sizeof(coordT));
  qh gm_row= (coordT **)qh_memalloc((qh hull_dim+1) * sizeof(coordT *));
} /* initqhull_buffers */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initqhull_globals">-</a>
  
  qh_initqhull_globals( points, numpoints, dim, ismalloc )
    initialize globals
    if ismalloc 
      points were malloc'd and qhull should free at end

  returns:
    sets qh.first_point, num_points, input_dim, hull_dim and others
    seeds random number generator (seed=1 if tracing)
    modifies qh.hull_dim if ((qh.DELAUNAY and qh.PROJECTdelaunay) or qh.PROJECTinput)
    adjust user flags as needed
    also checks DIM3 dependencies and constants

  notes:
    do not use qh_point() since an input transformation may move them elsewhere
    
  see:
    qh_initqhull_start() sets default values for non-zero globals
    
  design:
    initialize points array from input arguments
    test for qh.ZEROcentrum 
      (i.e., use opposite vertex instead of cetrum for convexity testing)
    test for qh.PRINTgood (i.e., only print 'good' facets)
    initialize qh.CENTERtype, qh.normal_size,
      qh.center_size, qh.TRACEpoint/level,
    initialize and test random numbers 
    check for conflicting print output options
*/
void qh_initqhull_globals (coordT *points, int numpoints, int dim, boolT ismalloc) {
  int seed, pointsneeded, extra= 0, i, randi, k;
  boolT printgeom= False, printmath= False, printcoplanar= False;
  realT randr;
  realT factorial;
#if !defined(PSXII) && !defined(GEKKO)
  time_t timedata;
#endif

  trace0((qh ferr, "qh_initqhull_globals: for %s | %s\n", qh rbox_command, 
      qh qhull_command));
  qh POINTSmalloc= ismalloc;
  qh first_point= points;
  qh num_points= numpoints;
  qh hull_dim= qh input_dim= dim;
  if (!qh NOpremerge && !qh MERGEexact && !qh PREmerge && qh JOGGLEmax > REALmax/2) {
    qh MERGING= True;
    if (qh hull_dim <= 4) {
      qh PREmerge= True;
      qh_option ("_pre-merge", nullptr, nullptr);
    }else {
      qh MERGEexact= True;
      qh_option ("_merge-exact", nullptr, nullptr);
    }
  }
  if (!qh NOpremerge && qh JOGGLEmax > REALmax/2) {
#ifdef qh_NOmerge
    qh JOGGLEmax= 0.0;
#endif
  }
  if (qh JOGGLEmax < REALmax/2 && qh DELAUNAY && !qh SCALEinput && !qh SCALElast) {
    qh SCALElast= True;
    qh_option ("Qbbound-last-qj", nullptr, nullptr);
  }
  if (qh MERGING && !qh POSTmerge && qh premerge_cos > REALmax/2 
  && qh premerge_centrum == 0) {
    qh ZEROcentrum= True;
    qh ZEROall_ok= True;
    qh_option ("_zero-centrum", nullptr, nullptr);
  }
  if (qh JOGGLEmax < REALmax/2 && REALepsilon > 2e-8 && qh PRINTprecision)
    fprintf(qh ferr, "qhull warning: real epsilon, %2.2g, is probably too large for joggle ('QJn')\nRecompile with double precision reals (see user.h).\n",
          REALepsilon);
#ifdef qh_NOmerge
  if (qh MERGING) {
    ivp_message( "qhull input error: merging not installed (qh_NOmerge + 'Qx', 'Cn' or 'An')\n");
    qh_errexit (qh_ERRinput, NULL, NULL);
  }
#endif
  if ((qh KEEParea || qh KEEPminArea < REALmax/2 || qh KEEPmerge || qh DELAUNAY)
   && !(qh PRINTgood || qh PRINTneighbors)) {
    qh PRINTgood= True;
    qh_option ("Pgood", nullptr, nullptr);
  }
  if (qh DELAUNAY && qh KEEPcoplanar && !qh KEEPinside) {
    qh KEEPinside= True;
    qh_option ("Qinterior-keep", nullptr, nullptr);
  }
  if (qh DELAUNAY && qh HALFspace) {
    ivp_message( "qhull input error: can not use Delaunay ('d') or Voronoi ('v') with halfspace intersection ('H')\n");
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  if (!qh DELAUNAY && (qh UPPERdelaunay || qh ATinfinity)) {
    ivp_message( "qhull input error: use upper-Delaunay ('Qu') or infinity-point ('Qz') with Delaunay ('d') or Voronoi ('v')\n");
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  if (qh UPPERdelaunay && qh ATinfinity) {
    ivp_message( "qhull input error: can not use infinity-point ('Qz') with upper-Delaunay ('Qu')\n");
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  if (qh SCALElast && !qh DELAUNAY)
    ivp_message( "qhull input warning: option 'Qbb' (scale-last-coordinate) is normally used with 'd' or 'v'\n");
  qh DOcheckmax= (!qh FORCEoutput && !qh SKIPcheckmax && qh MERGING );
  qh KEEPnearinside= (qh DOcheckmax && !(qh KEEPinside && qh KEEPcoplanar) 
                          && !qh NOnearinside);
  if (qh MERGING)
    qh CENTERtype= qh_AScentrum;
  else if (qh VORONOI)
    qh CENTERtype= qh_ASvoronoi;
  if (qh TESTvneighbors && !qh MERGING) {
    fprintf(qh ferr, "qhull input error: test vertex neighbors ('Qv') needs a merge option\n");
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  if (qh PROJECTinput || (qh DELAUNAY && qh PROJECTdelaunay)) {
    qh hull_dim -= qh PROJECTinput;
    if (qh DELAUNAY) {
      qh hull_dim++;
      extra= 1;
    }
  }
  if (qh hull_dim <= 1) {
    fprintf(qh ferr, "qhull error: dimension %d must be > 1\n", qh hull_dim);
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  for (k= 2, factorial=1.0; k < qh hull_dim; k++)
    factorial *= k;
  qh AREAfactor= 1.0 / factorial;
  trace2((qh ferr, "qh_initqhull_globals: initialize globals.  dim %d numpoints %d malloc? %d projected %d to hull_dim %d\n",
	dim, numpoints, ismalloc, qh PROJECTinput, qh hull_dim));
  qh normal_size= qh hull_dim * sizeof(coordT);
  qh center_size= qh normal_size - sizeof(coordT);
  pointsneeded= qh hull_dim+1;
  if (qh hull_dim > qh_DIMmergeVertex) {
    qh MERGEvertices= False;
    qh_option ("Q3-no-merge-vertices-dim-high", nullptr, nullptr);
  }
  if (qh GOODpoint)
    pointsneeded++;
#ifdef qh_NOtrace
  if (qh IStracing) {
    ivp_message( "qhull input error: tracing is not installed (qh_NOtrace in user.h)");
    qh_errexit (qh_ERRqhull, NULL, NULL);
  } 
#endif
  if (qh RERUN > 1) {
    qh TRACElastrun= qh IStracing; /* qh_build_withrestart duplicates next conditional */
    if (qh IStracing != -1)
      qh IStracing= 0;
  }else if (qh TRACEpoint != -1 || qh TRACEdist < REALmax/2 || qh TRACEmerge) {
    qh TRACElevel= (qh IStracing? qh IStracing : 3);
    qh IStracing= 0;
  }
  if (qh ROTATErandom == 0 || qh ROTATErandom == -1) {
#if !defined(PSXII) && !defined(GEKKO)
    seed= time (&timedata);
#endif
    if (qh ROTATErandom  == -1) {
      seed= -seed;
      qh_option ("QRandom-seed", &seed, nullptr);
    }else 
      qh_option ("QRotate-random", &seed, nullptr);
    qh ROTATErandom= seed;
  }
  seed= qh ROTATErandom;
  if (seed == INT_MIN)    /* default value */
    seed= 1;
  else if (seed < 0)
    seed= -seed;
  qh_RANDOMseed_(seed);
  randr= 0.0;
  for (i= 1000; i--; ) {
    randi= qh_RANDOMint;
    randr += randi;
    if (randi > qh_RANDOMmax) {
      ivp_message( "\
qhull configuration error (qh_RANDOMmax in user.h):\n\
   random integer %d > qh_RANDOMmax (%.8g)\n",
	       randi, qh_RANDOMmax);
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }
  }
  qh_RANDOMseed_(seed);
  randr = randr/1000;
  if (randr < qh_RANDOMmax/10
  || randr > qh_RANDOMmax * 5)
    ivp_message( "\
qhull configuration warning (qh_RANDOMmax in user.h):\n\
   average of 1000 random integers (%.2g) is much different than expected (%.2g).\n\
   Is qh_RANDOMmax (%f) wrong?\n",
	     randr, qh_RANDOMmax/2, qh_RANDOMmax);
  qh RANDOMa= 2.0 * qh RANDOMfactor/qh_RANDOMmax;
  qh RANDOMb= 1.0 - qh RANDOMfactor;
  if (qh_HASHfactor < 1.1) {
    fprintf(qh ferr, "qhull internal error (qh_initqhull_globals): qh_HASHfactor %d must be at least 1.1.  Qhull uses linear hash probing\n",
      qh_HASHfactor);
    qh_errexit (qh_ERRqhull, nullptr, nullptr);
  }
  if (numpoints+extra < pointsneeded) {
    fprintf(qh ferr,"qhull input error: not enough points (%d) to construct initial simplex (need %d)\n",
	    numpoints, pointsneeded);
    qh_errexit(qh_ERRinput, nullptr, nullptr);
  }
  if (qh PRINTtransparent) {
    if (qh hull_dim != 4 || !qh DELAUNAY || qh VORONOI || qh DROPdim >= 0) {
      fprintf(qh ferr,"qhull input error: transparent Delaunay ('Gt') needs 3-d Delaunay ('d') w/o 'GDn'\n");
      qh_errexit(qh_ERRinput, nullptr, nullptr);
    }
    qh DROPdim = 3;
    qh PRINTridges = True;
  }
  for (i= qh_PRINTEND; i--; ) {
    if (qh PRINTout[i] == qh_PRINTgeom)
      printgeom= True;
    else if (qh PRINTout[i] == qh_PRINTmathematica)
      printmath= True;
    else if (qh PRINTout[i] == qh_PRINTcoplanars)
      printcoplanar= True;
    else if (qh PRINTout[i] == qh_PRINTpointnearest)
      printcoplanar= True;
    else if (qh PRINTout[i] == qh_PRINTpointintersect && !qh HALFspace) {
      ivp_message( "qhull input error: option 'Fp' is only used for \nhalfspace intersection ('Hn,n,n').\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }else if (qh PRINTout[i] == qh_PRINTtriangles 
    && (qh HALFspace || qh VORONOI)) {
      ivp_message( "qhull input error: option 'Ft' is not available for Voronoi vertices or halfspace intersection\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }else if (qh PRINTout[i] == qh_PRINTcentrums && qh VORONOI) {
      ivp_message( "qhull input error: option 'Fc' is not available for Voronoi vertices ('v')\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }else if (qh PRINTout[i] == qh_PRINTvertices) {
      if (qh VORONOI)
        qh_option ("Fvoronoi", nullptr, nullptr);
      else
        qh_option ("Fvertices", nullptr, nullptr);
    }
  }
  if (!qh KEEPcoplanar && !qh KEEPinside && !qh ONLYgood) {
    if ((qh PRINTcoplanar && qh PRINTspheres) || printcoplanar)
      ivp_message( "qhull input warning: options 'Fc', 'FP', and 'Gp' need option 'Qc' or 'Qi' to record coplanar/inside points\n");
  }
  if (printmath && (qh hull_dim > 3 || qh VORONOI || qh HALFspace)) {
    ivp_message( "qhull input error: Mathematica output is only available for 2-d and 3-d convex hulls and Delaunay triangulations\n");
    qh_errexit (qh_ERRinput, nullptr, nullptr);
  }
  if (printgeom) {
    if (qh hull_dim > 4) {
      ivp_message( "qhull input error: Geomview output is only available for 2-d, 3-d and 4-d\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }
    if (qh PRINTnoplanes && !(qh PRINTcoplanar + qh PRINTcentrums
     + qh PRINTdots + qh PRINTspheres + qh DOintersections + qh PRINTridges)) {
      ivp_message( "qhull input error: no output specified for Geomview\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }
    if (qh VORONOI && (qh hull_dim > 3 || qh DROPdim >= 0)) {
      ivp_message( "qhull input error: Geomview output for Voronoi diagrams only for 2-d\n");
      qh_errexit (qh_ERRinput, nullptr, nullptr);
    }
    /* can not warn about furthest-site Geomview output: no lower_threshold */
    if (qh hull_dim == 4 && qh DROPdim == -1 &&
	(qh PRINTcoplanar || qh PRINTspheres || qh PRINTcentrums)) {
      ivp_message( "qhull input warning: coplanars, vertices, and centrums output not\n\
available for 4-d output (ignored).  Could use 'GDn' instead.\n");
      qh PRINTcoplanar= qh PRINTspheres= qh PRINTcentrums= False;
    }
  }
  qh PRINTdim= qh hull_dim;
  if (qh DROPdim >=0) {    /* after Geomview checks */
    if (qh DROPdim < qh hull_dim) {
      qh PRINTdim--;
      if (!printgeom || qh hull_dim < 3) 
        ivp_message( "qhull input warning: drop dimension 'GD%d' is only available for 3-d/4-d Geomview\n", qh DROPdim);
    }else
      qh DROPdim= -1;
  }else if (qh VORONOI) {
    qh DROPdim= qh hull_dim-1;
    qh PRINTdim= qh hull_dim-1; 
  }
} /* initqhull_globals */
 
/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initqhull_mem">-</a>
  
  qh_initqhull_mem(  )
    initialize mem.c for qhull
    qh.hull_dim and qh.normal_size determine some of the allocation sizes
    if qh.MERGING, 
      includes ridgeT
    calls qh_user_memsizes() to add up to 10 additional sizes for quick allocation 
      (see numsizes below)

  returns:
    mem.c already for qh_memalloc/qh_memfree (errors if called beforehand)

  notes:
    qh_produceoutput() prints memsizes

*/
void qh_initqhull_mem (void) {
  int numsizes;
  int i;

  numsizes= 8+10;
  qh_meminitbuffers (qh IStracing, qh_MEMalign, numsizes, 
                     qh_MEMbufsize,qh_MEMinitbuf);
  qh_memsize(sizeof(vertexT));
  if (qh MERGING) {
    qh_memsize(sizeof(ridgeT));
    qh_memsize(sizeof(mergeT));
  }
  qh_memsize(sizeof(facetT));
  qh_memsize(sizeof(hashentryT));
  i= sizeof(setT) + (qh hull_dim - 1) * SETelemsize;  /* ridge.vertices */
  qh_memsize(i);
  qh_memsize(qh normal_size);        /* normal */
  i += SETelemsize;                 /* facet.vertices, .ridges, .neighbors */
  qh_memsize(i);
  qh_user_memsizes();
  qh_memsetup();
} /* initqhull_mem */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initqhull_start">-</a>
  
  qh_initqhull_start( infile, outfile, errfile )
    start initialization of qhull
    initialize statistics, stdio, default values for global variables

  see:
    qh_maxmin() determines the precision constants
*/
void qh_initqhull_start (FILE *infile, FILE *outfile, FILE *errfile) {

  qh_CPUclock; /* start the clock */
#if qh_QHpointer
  if (!(qh_qh= (qhT *)p_malloc (sizeof(qhT)))) {
    fprintf (errfile, "qhull error (qh_initqhull_globals): insufficient memory\n");
    exit (qh_ERRmem);  /* no error handler */
  }
  memset((char *)qh_qh, 0, sizeof(qhT));   /* every field is 0, FALSE, NULL */
#else
  memset((char *)&qh_qh, 0, sizeof(qhT));
#endif
  strcat (qh qhull, "qhull");
  qh_initstatistics();
  qh ANGLEmerge= True;
  qh DROPdim= -1;
  qh ferr= errfile;
  qh fin= infile;
  qh fout= outfile;
  qh furthest_id= -1;
  qh JOGGLEmax= REALmax;
  qh KEEPminArea = REALmax;
  qh last_low= REALmax;
  qh last_high= REALmax;
  qh last_newhigh= REALmax;
  qh max_outside= 0.0;
  qh max_vertex= 0.0;
  qh MAXabs_coord= 0.0;
  qh MAXsumcoord= 0.0;
  qh MAXwidth= -REALmax;
  qh MERGEindependent= True;
  qh MINdenom_1= 0.0;
  qh MINoutside= 0.0;
  qh MINvisible= REALmax;
  qh MAXcoplanar= REALmax;
  qh outside_err= REALmax;
  qh premerge_centrum= 0.0;
  qh premerge_cos= REALmax;
  qh PRINTprecision= True;
  qh PRINTradius= 0.0;
  qh postmerge_cos= REALmax;
  qh postmerge_centrum= 0.0;
  qh ROTATErandom= INT_MIN;
  qh MERGEvertices= True;
  qh totarea= 0.0;
  qh totvol= 0.0;
  qh TRACEdist= REALmax;
  qh TRACEpoint= -1;  /* recompile to trace a point */
  qh tracefacet_id= UINT_MAX;  /* recompile to trace a facet */
  qh tracevertex_id= UINT_MAX; /* recompile to trace a vertex */
  qh_RANDOMseed_(1);
} /* initqhull_start */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="initthresholds">-</a>
  
  qh_initthresholds( commandString )
    set thresholds for printing and scaling from commandString

  returns:
    sets qh.GOODthreshold or qh.SPLITthreshold if 'Pd0D1' used

  see: 
    qh_initflags(), 'Qbk' 'QBk' 'Pdk' and 'PDk'
    see 'prompt' in unix.c for documentation

  design:
    for each 'Pdn' or 'PDn' option
      check syntax
      set qh.lower_threshold or qh.upper_threshold
    set qh.GOODthreshold if an unbounded threshold is used
    set qh.SPLITthreshold if a bounded threshold is used  
*/
void qh_initthresholds(char *command) {
  realT value;
  int index, maxdim, k;
  char *s= command;
  char key;
  
  maxdim= qh input_dim;
  if (qh DELAUNAY && (qh PROJECTdelaunay || qh PROJECTinput))
    maxdim++;
  while (*s) {
    if (*s == '-')
      s++;
    if (*s == 'P') {
      s++;
      while (*s && !isspace(key= *s++)) {
	if (key == 'd' || key == 'D') {
	  if (!isdigit(*s)) {
	    fprintf(qh ferr, "qhull warning: no dimension given for Print option '%c' at: %s.  Ignored\n",
		    key, s-1);
	    continue;
	  }
	  index= qh_strtol (s, &s);
	  if (index >= qh hull_dim) {
	    fprintf(qh ferr, "qhull warning: dimension %d for Print option '%c' is >= %d.  Ignored\n", 
	        index, key, qh hull_dim);
	    continue;
	  }
	  if (*s == ':') {
	    s++;
	    value= qh_strtod(s, &s);
	    if (fabs((double)value) > 1.0) {
	      fprintf(qh ferr, "qhull warning: value %2.4g for Print option %c is > +1 or < -1.  Ignored\n", 
	              value, key);
	      continue;
	    }
	  }else
	    value= 0.0;
	  if (key == 'd')
	    qh lower_threshold[index]= value;
	  else
	    qh upper_threshold[index]= value;
	}
      }
    }else if (*s == 'Q') {
      s++;
      while (*s && !isspace(key= *s++)) {
	if (key == 'b' && *s == 'B') {
	  s++;
	  for (k=maxdim; k--; ) {
	    qh lower_bound[k]= -qh_DEFAULTbox;
	    qh upper_bound[k]= qh_DEFAULTbox;
	  }
	}else if (key == 'b' && *s == 'b')
	  s++;
	else if (key == 'b' || key == 'B') {
	  if (!isdigit(*s)) {
	    fprintf(qh ferr, "qhull warning: no dimension given for Qhull option %c.  Ignored\n",
		    key);
	    continue;
	  }
	  index= qh_strtol (s, &s);
	  if (index >= maxdim) {
	    fprintf(qh ferr, "qhull warning: dimension %d for Qhull option %c is >= %d.  Ignored\n", 
	        index, key, maxdim);
	    continue;
	  }
	  if (*s == ':') {
	    s++;
	    value= qh_strtod(s, &s);
	  }else if (key == 'b')
	    value= -qh_DEFAULTbox;
	  else
	    value= qh_DEFAULTbox;
	  if (key == 'b')
	    qh lower_bound[index]= value;
	  else
	    qh upper_bound[index]= value;
	}
      }
    }else {
      while (*s && !isspace (*s))
        s++;
    }
    while (isspace (*s))
      s++;
  }
  for (k= qh hull_dim; k--; ) {
    if (qh lower_threshold[k] > -REALmax/2) {
      qh GOODthreshold= True;
      if (qh upper_threshold[k] < REALmax/2) {
        qh SPLITthresholds= True;
        qh GOODthreshold= False;
        break;
      }
    }else if (qh upper_threshold[k] < REALmax/2)
      qh GOODthreshold= True;
  }
} /* initthresholds */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="option">-</a>
  
  qh_option( option, intVal, realVal )
    add an option description to qh.qhull_options

  notes:
    will be printed with statistics ('Ts') and errors
    strlen(option) < 40
*/
void qh_option (const char *option, int *i, realT *r) {
  char buf[200];
  int len, maxlen;

  sprintf (buf, "  %s", option);
  if (i)
    sprintf (buf+strlen(buf), " %d", *i);
  if (r)
    sprintf (buf+strlen(buf), " %2.2g", *r);
  len= strlen(buf);
  qh qhull_optionlen += len;
  maxlen= sizeof (qh qhull_options) - len -1;
  maximize_(maxlen, 0);
  if (qh qhull_optionlen >= 80 && maxlen > 0) {
    qh qhull_optionlen= len;
    strncat (qh qhull_options, "\n", maxlen--);
  }    
  strncat (qh qhull_options, buf, maxlen);
} /* option */

#if qh_QHpointer
/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="restore_qhull">-</a>
  
  qh_restore_qhull( oldqh )
    restores a previously saved qhull
    also restores qh_qhstat and qhmem.tempstack

  notes:
    errors if current qhull hasn't been saved or freed
    uses qhmem for error reporting
 
  NOTE 1998/5/11:  
    Freeing memory after qh_save_qhull and qh_restore_qhull 
    is complicated.  The procedures will be redesigned.
    
  see:
    qh_save_qhull()
*/
void qh_restore_qhull (qhT **oldqh) {

  if (*oldqh && strcmp ((*oldqh)->qhull, "qhull")) {
    fprintf (qhmem.ferr, "qhull internal error (qh_restore_qhull): %p is not a qhull data structure\n",
                  *oldqh);
    qh_errexit (qh_ERRqhull, NULL, NULL);
  }
  if (qh_qh) {
    fprintf (qhmem.ferr, "qhull internal error (qh_restore_qhull): did not save or free existing qhull\n");
    qh_errexit (qh_ERRqhull, NULL, NULL);
  }
  if (!*oldqh || !(*oldqh)->old_qhstat) {
    fprintf (qhmem.ferr, "qhull internal error (qh_restore_qhull): did not previously save qhull %p\n",
                  *oldqh);
    qh_errexit (qh_ERRqhull, NULL, NULL);
  }
  qh_qh= *oldqh;
  *oldqh= NULL;
  qh_qhstat= qh old_qhstat;
  qhmem.tempstack= qh old_tempstack;
  trace1((qh ferr, "qh_restore_qhull: restored qhull from %p\n", *oldqh));
} /* restore_qhull */

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="save_qhull">-</a>
  
  qh_save_qhull(  )
    saves qhull for a later qh_restore_qhull
    also saves qh_qhstat and qhmem.tempstack

  returns:
    qh_qh=NULL

  notes:
    need to initialize qhull or call qh_restore_qhull before continuing
    
  NOTE 1998/5/11:  
    Freeing memory after qh_save_qhull and qh_restore_qhull 
    is complicated.  The procedures will be redesigned.
    
  see:
    qh_restore_qhull()
*/
qhT *qh_save_qhull (void) {
  qhT *oldqh;

  trace1((qhmem.ferr, "qh_save_qhull: save qhull %p\n", qh_qh));
  if (!qh_qh) {
    fprintf (qhmem.ferr, "qhull internal error (qh_save_qhull): qhull not initialized\n");
    qh_errexit (qh_ERRqhull, NULL, NULL);
  }
  qh old_qhstat= qh_qhstat;
  qh_qhstat= NULL;
  qh old_tempstack= qhmem.tempstack;
  qhmem.tempstack= NULL;
  oldqh= qh_qh;
  qh_qh= NULL;
  return oldqh;
} /* save_qhull */

#endif

/*-<a                             href="qh-c.htm#global"
  >-------------------------------</a><a name="strtol">-</a>
  
  qh_strtol( s, endp) qh_strtod( s, endp)
    internal versions of strtol() and strtod()
    does not skip trailing spaces
  notes:
    some implementations of strtol()/strtod() skip trailing spaces
*/
double qh_strtod (const char *s, char **endp) {
  double result;

  result= strtod (s, endp);
  if (s < (*endp) && (*endp)[-1] == ' ')
    (*endp)--;
  return result;
} /* strtod */

int qh_strtol (const char *s, char **endp) {
  int result;

  result= (int) strtol (s, endp, 10);
  if (s< (*endp) && (*endp)[-1] == ' ')
    (*endp)--;
  return result;
} /* strtol */
