{
    line = $0;

    if (index(line, "#") != 0)
    {
        split(line, arr, "#");
        line = arr[1];
    }

    if (index(line, "=") != 0)
    {
        split(line, arr, "=");

        if (arr[1] == key_name)
        {
            print arr[2]
        }
    }
}
