#!/usr/bin/perl -w
#
my $prefix = ".." ;
my $pwd = `pwd 2> /dev/null` ;
chomp($pwd) ;

sub GetVersion {
  while (1) {
    printf "Major:" ;
    my $major = <> ;
    printf "Minor:" ;
    my $minor = <> ;
    printf "Patch level:" ;
    my $patch = <> ;
    my @v = map {chomp;$_} ($major,$minor,$patch) ;
    printf "Is ".join("_",("RELEASE",@v))." correct? (y/n)" ;
    my $ans = <> ;
    redo unless $ans =~ m/^[yY]/ ;
    return @v ;
  }
} ;

sub ConfigureAC {
  my ($maj,$min,$pat) = @_ ;
  printf "RELEASE -- updating configure.ac\n" ;
  `/usr/bin/perl -i.old -pe 's\{VERSION_MAJOR=.*?\$\}\{VERSION_MAJOR=$maj}' configure.ac` ;
  `/usr/bin/perl -i.old -pe 's\{VERSION_MINOR=.*?\$\}\{VERSION_MINOR=$min}' configure.ac` ;
  `/usr/bin/perl -i.old -pe 's\{PATCHLEVEL=.*?\$\}\{PATCHLEVEL=$pat}' configure.ac` ;
}

sub PerModule {
  my ($mod,@v) = @_ ;
  # pushd
  chdir $prefix."/".$mod or die "Cannot change to dir $prefix from $pwd" ;

  printf "RELEASE -- cvs branch tagging ".join("_",("RELEASE",@v,"RC"))."\n" ;
  system("cvs","tag","-b",join("_",("RELEASE",@v,"RC")) ) ;
  printf "RELEASE -- cvs updating ".join("_",("RELEASE",@v,"RC"))."\n" ;
  system("cvs","update","-r",join("_",("RELEASE",@v,"RC")) ) ;

  # change the configure.ac and re-bootstrap
  ConfigureAC(@v) ;

  # make process
  printf "RELEASE -- make clean\n" ;
  system("make","clean") ;
  printf "RELEASE -- make distclean\n" ;
  system("make","distclean") ;
  printf "RELEASE -- bootstrapping\n" ;
  `./bootstrap` ;
  printf "RELEASE -- make tarball\n" ;
  system("make","dist") ;
  printf "RELEASE -- make rpm\n" ;
  system("make","rpm") ;

  printf $mod."-".$v[0].".".$v[1]."p".$v[2].".tar.gz = ".system("ls","*.tar.gz") ;
  
  printf "Commit now\n" ;
  system( "cvs","-z3","commit","-R","-m", "''". "'.'");

  printf "RELEASE -- cvs branch tagging ".join("_",("RELEASE",@v))."\n" ;
  #system("cvs","tag","-b",join("_",("RELEASE",@v,"RC")) ) ;

  #popd
  chdir $pwd or die "Cannot change back to dir $pwd from $prefix: $!" ;

} ;

# pushd
chdir $prefix or die "Cannot change to dir $prefix from $pwd" ;

printf "OWFS releaser\n" ;
my @v = GetVersion() ;

printf "RELEASE -- release name\n" ;
printf join("_",("RELEASE",@v,"RC"))."\n" ;
PerModule("owlib",@v) ;
PerModule("owfs",@v) ;
PerModule("owhttpd",@v) ;
