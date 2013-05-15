#!/usr/bin/awk -f
BEGIN{
  count=0;
  prevline=""
}
{
  #print $0;
  if ( $0 == prevline ){
    count++;
  } else {
    print  prevline " = " count;
    count=1;
  }
prevline=$0;
}
