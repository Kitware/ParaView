#!/usr/bin/perl -n -i.bak

BEGIN 
{
    $StopPrinting = 0;
}

$line = $_;
if ( $line =~ "Start NavBar"  )
{
    $StopPrinting = 1;
}
elsif ( $line =~ "End NavBar"  )
{
    $StopPrinting = 0;
}
else
{
    if ($StopPrinting == 0)
    {
	print;
    }
}


    
