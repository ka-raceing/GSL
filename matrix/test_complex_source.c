void FUNCTION (test, func) (void);
void FUNCTION (test, trap) (void);
void FUNCTION (test, text) (void);
void FUNCTION (test, binary) (void);

void
FUNCTION (test, func) (void)
{

  size_t i, j;
  int k = 0;

  TYPE (gsl_block) * b = FUNCTION (gsl_block, alloc) (N * M);
  TYPE (gsl_matrix) * m = FUNCTION (gsl_matrix, alloc) (b, 0, N, M, M);

  gsl_test (m->data != b->data, NAME (gsl_matrix) "_alloc returns valid pointer");
  gsl_test (m->size1 != N, NAME (gsl_matrix) "_alloc returns valid size1");
  gsl_test (m->size2 != M, NAME (gsl_matrix) "_alloc returns valid size2");
  gsl_test (m->dim2 != M, NAME (gsl_matrix) "_alloc returns valid dim2");

  for (i = 0; i < N; i++)
    {
      for (j = 0; j < M; j++)
	{
	  BASE z;
	  k++;
	  GSL_REAL(z) = k;
	  GSL_IMAG(z) = k + 1000;
	  FUNCTION (gsl_matrix, set) (m, i, j, z);
	}
    }

  status = 0;
  k = 0;
  for (i = 0; i < N; i++)
    {
      for (j = 0; j < M; j++)
	{
	  k++;
	  if (m->data[2 * (i * M + j)] != k || 
	      m->data[2 * (i * M + j) + 1] != k + 1000)
	    status = 1;
	}
    }
  
  gsl_test (status, NAME (gsl_matrix) "_set writes into array correctly");

  status = 0;
  k = 0;
  for (i = 0; i < N; i++)
    {
      for (j = 0; j < M; j++)
	{
	  BASE z = FUNCTION (gsl_matrix, get) (m, i, j);
	  k++;
	  if (GSL_REAL(z) != k || GSL_IMAG(z) != k + 1000)
	    status = 1;
	}
    }
  gsl_test (status, NAME (gsl_matrix) "_get reads from array correctly");

  FUNCTION (gsl_matrix, free) (m);	/* free whatever is in m */
  FUNCTION (gsl_block, free) (b);

}

#if !(defined(USES_LONGDOUBLE) && !defined(HAVE_PRINTF_LONGDOUBLE))
void
FUNCTION (test, text) (void)
{
  TYPE (gsl_block) * b = FUNCTION (gsl_block, alloc) (N * M);
  TYPE (gsl_matrix) * m = FUNCTION (gsl_matrix, alloc) (b, 0, N, M, M);

  size_t i, j;
  int k = 0;

  {
    FILE *f = fopen ("test.txt", "w");
    k = 0;
    for (i = 0; i < N; i++)
      {
	for (j = 0; j < M; j++)
	  {
	    BASE z;
	    k++;
	    GSL_REAL(z) = k;
	    GSL_IMAG(z) = k + 1000;
	    FUNCTION (gsl_matrix, set) (m, i, j, z);
	  }
      }

    FUNCTION (gsl_matrix, fprintf) (f, m, OUT_FORMAT);

    fclose (f);
  }

  {
    FILE *f = fopen ("test.txt", "r");
    TYPE (gsl_block) * bb = FUNCTION (gsl_block, alloc) (N * M);
    TYPE (gsl_matrix) * mm = FUNCTION (gsl_matrix, alloc) (b, 0, N, M, M);
    status = 0;

    FUNCTION (gsl_matrix, fscanf) (f, mm);
    k = 0;
    for (i = 0; i < N; i++)
      {
	for (j = 0; j < M; j++)
	  {
	    k++;
	    if (mm->data[2 * (i * M + j)] != k || mm->data[2 * (i * M + j) + 1] != k + 1000)
	      status = 1;
	  }
      }

    gsl_test (status, NAME (gsl_matrix) "_fprintf and fscanf work correctly");

    fclose (f);
    FUNCTION (gsl_matrix, free) (mm);
    FUNCTION (gsl_block, free) (bb);
  }

  FUNCTION (gsl_matrix, free) (m);
  FUNCTION (gsl_block, free) (b);
}
#endif

void
FUNCTION (test, binary) (void)
{
  TYPE (gsl_block) * b = FUNCTION (gsl_block, alloc) (N * M);
  TYPE (gsl_matrix) * m = FUNCTION (gsl_matrix, alloc) (b, 0, N, M, M);

  size_t i, j;
  int k = 0;

  {
    FILE *f = fopen ("test.dat", "w");
    k = 0;
    for (i = 0; i < N; i++)
      {
	for (j = 0; j < M; j++)
	  {
	    BASE z;
	    k++;
	    GSL_REAL(z) = k;
	    GSL_IMAG(z) = k + 1000;
	    FUNCTION (gsl_matrix, set) (m, i, j, z);
	  }
      }

    FUNCTION (gsl_matrix, fwrite) (f, m);

    fclose (f);
  }

  {
    FILE *f = fopen ("test.dat", "r");
    TYPE (gsl_block) * bb = FUNCTION (gsl_block, alloc) (N * M);
    TYPE (gsl_matrix) * mm = FUNCTION (gsl_matrix, alloc) (b, 0, N, M, M);
    status = 0;

    FUNCTION (gsl_matrix, fread) (f, mm);
    k = 0;
    for (i = 0; i < N; i++)
      {
	for (j = 0; j < M; j++)
	  {
	    k++;
	    if (mm->data[2 * (i * M + j)] != k || mm->data[2 * (i * M + j) + 1] != k + 1000)
	      status = 1;
	  }
      }

    gsl_test (status, NAME (gsl_matrix) "_write and read work correctly");

    fclose (f);
    FUNCTION (gsl_matrix, free) (mm);
    FUNCTION (gsl_block, free) (bb);
  }

  FUNCTION (gsl_matrix, free) (m);
  FUNCTION (gsl_block, free) (b);
}

void
FUNCTION (test, trap) (void)
{
  TYPE (gsl_block) * bc = FUNCTION (gsl_block, alloc) (N * M);
  TYPE (gsl_matrix) * mc = FUNCTION (gsl_matrix, alloc) (bc, 0, N, M, M);
  size_t i = 0, j = 0;

  BASE z = {{1.2, 3.4}};
  BASE z1;

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, i - 1, j, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 1st index below lower bound");

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, i, j - 1, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 2nd index below lower bound");

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, N + 1, 0, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 1st index above upper bound");

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, 0, M + 1, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 2nd index above upper bound");

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, N, 0, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 1st index at upper bound");

  status = 0;
  FUNCTION (gsl_matrix, set) (mc, 0, M, z);
  gsl_test (!status,
	    NAME (gsl_matrix) "_set traps 2nd index at upper bound");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, i - 1, 0);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 1st index below lower bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 1st index below l.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 1st index below l.b.");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, 0, j - 1);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 2nd index below lower bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 2nd index below l.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 2nd index below l.b.");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, N + 1, 0);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 1st index above upper bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 1st index above u.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 1st index above u.b.");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, 0, M + 1);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 2nd index above upper bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 2nd index above u.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 2nd index above u.b.");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, N, 0);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 1st index at upper bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 1st index at u.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 1st index at u.b.");

  status = 0;
  z1 = FUNCTION (gsl_matrix, get) (mc, 0, M);
  gsl_test (!status,
	    NAME (gsl_matrix) "_get traps 2nd index at upper bound");
  gsl_test (GSL_REAL(z1) != 0,
	    NAME (gsl_matrix) "_get, zero real for 2nd index at u.b.");
  gsl_test (GSL_IMAG(z1) != 0,
	    NAME (gsl_matrix) "_get, zero imag for 2nd index at u.b.");

 FUNCTION (gsl_matrix, free) (mc);
 FUNCTION (gsl_block, free) (bc);
}




