int
FUNCTION (gsl_matrix, copy_row) (TYPE (gsl_vector) * v,
                                 const TYPE (gsl_matrix) * m,
				 const size_t i)
{
  const size_t column_length = m->size1;
  const size_t row_length = m->size2;

  if (i >= column_length)
    {
      GSL_ERROR ("row index is out of range", GSL_EINVAL);
    }

  if (v->size != row_length)
    {
      GSL_ERROR ("matrix row size and vector length are not equal",
		 GSL_EBADLEN);
    }

  {
    ATOMIC *row_data = m->data + MULTIPLICITY * i * row_length;
    const size_t stride = v->stride ;
    size_t j;

    for (j = 0; j < MULTIPLICITY * row_length; j++)
      {
	v->data[stride * j] = row_data[j];
      }
  }

  return GSL_SUCCESS;
}

int
FUNCTION (gsl_matrix, copy_col) (TYPE (gsl_vector) * v,
                                 const TYPE (gsl_matrix) * m,
				 const size_t j)
{
  const size_t column_length = m->size1;
  const size_t row_length = m->size2;

  if (j >= row_length)
    {
      GSL_ERROR ("column index is out of range", GSL_EINVAL);
    }

  if (v->size != column_length)
    {
      GSL_ERROR ("matrix column size and vector length are not equal",
		 GSL_EBADLEN);
    }


  {
    ATOMIC *column_data = m->data + MULTIPLICITY * j;
    const size_t stride = v->stride ;
    size_t i;

    for (i = 0; i < row_length; i++)
      {
	int k;
	for (k = 0; k < MULTIPLICITY; k++)
	  {
	    v->data[stride * MULTIPLICITY * j + k] =
	      column_data[MULTIPLICITY * i * row_length + k];
	  }
      }
  }

  return GSL_SUCCESS;
}

int
FUNCTION (gsl_matrix, set_row) (TYPE (gsl_matrix) * m,
				const size_t i,
				const TYPE (gsl_vector) * v)
{
  const size_t column_length = m->size1;
  const size_t row_length = m->size2;

  if (i >= column_length)
    {
      GSL_ERROR ("row index is out of range", GSL_EINVAL);
    }

  if (v->size != row_length)
    {
      GSL_ERROR ("matrix row size and vector length are not equal",
		 GSL_EBADLEN);
    }

  {
    const ATOMIC *v_data = v->data;
    ATOMIC *row_data = m->data + MULTIPLICITY * i * row_length;
    const size_t stride = v->stride ;
    size_t j;

    for (j = 0; j < MULTIPLICITY * row_length; j++)
      {
	row_data[j] = v_data[stride * j];
      }
  }

  return GSL_SUCCESS;
}

int
FUNCTION (gsl_matrix, set_col) (TYPE (gsl_matrix) * m,
				const size_t j,
				const TYPE (gsl_vector) * v)
{
  const size_t column_length = m->size1;
  const size_t row_length = m->size2;

  if (j >= row_length)
    {
      GSL_ERROR ("column index is out of range", GSL_EINVAL);
    }

  if (v->size != column_length)
    {
      GSL_ERROR ("matrix column size and vector length are not equal",
		 GSL_EBADLEN);
    }

  {
    const ATOMIC *v_data = v->data;
    ATOMIC *column_data = m->data + MULTIPLICITY * j;
    const size_t stride = v->stride ;
    size_t i;

    for (i = 0; i < column_length; i++)
      {
	column_data[MULTIPLICITY * i * row_length] = v_data[stride * i];
      }
  }

  return GSL_SUCCESS;
}
