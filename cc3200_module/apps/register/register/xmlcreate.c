#include <stdio.h>
const char *				/* O - Entity name or NULL */
mxmlEntityGetName(int val)		/* I - Character value */
{
  switch (val)
  {
    case '&' :
        return ("amp");

    case '<' :
        return ("lt");

    case '>' :
        return ("gt");

    case '\"' :
        return ("quot");
	case '\'':
		return ("apos");
    default :
        return (NULL);
  }
}

int				/* O - 0 on success, -1 on failure */
write_xml_string(
    const char      *s,			/* I - String to write */
    char            *dest)			/* I - Write pointer */
{
  const char	*name;			/* Entity name, if any */
  char* d = dest;
    if(s == NULL || dest == NULL)
        return -1;
  while (*s != 0)
  {
    if ((name = mxmlEntityGetName(*s)) != NULL)
    {
      *d++ = '&';

      while (*name)
      {
		*d++ = *name;
        name ++;
      }
	  
	  *d++ = ';';
    }
    else
	{
	   *d++ = *s;
	}
    s ++;
  }
	*d=0;
  return (0);
}

/*
int main()
{
	char dest [50];
	write_xml_string("toi&em", dest);
	printf("Output: \"%s\"\n",dest);
	return 0;
}*/

