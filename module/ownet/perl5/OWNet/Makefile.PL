use 5.008;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'OWNet',
    VERSION_FROM      => 'lib/OWNet.pm', # finds $VERSION
    PREREQ_PM         => {}, # e.g., Module::Name => 1.1
    ($] >= 5.008 ?     ## Add these new keywords supported since 5.008
      (ABSTRACT       => 'Light-weight connection to owserver to access the Dallas 1-wire system for monitoring and control',
       AUTHOR         => 'Paul H Alfille <paul.alfille@gmail.com>') : ()),
);
