function trimm(s)
{
    if (index(s, " ") != 0)
    {
        split(s, arr, " ");
        return arr[1];
    }
    return s;
}

{
    line = $0;
    comment = "";
    found = 0;

    if (index(line, "#") != 0)
    {
        split(line, arr, "#");
        line = arr[1];
        comment = arr[2];
    }

    if (index(line, "=") != 0)
    {
        split(line, arr, "=");
        if (trimm(arr[1]) == key_name)
        {
            line = trimm(arr[2]);  #found it
            found = 1;
        }
    }

    if (found == 0)
    {
        print $0;
    }
    else
    {
        #print "#old line:", $0
        if (comment != "")
            print key_name"="key_value" #"comment;
        else
            print key_name"="key_value;
    }
}
