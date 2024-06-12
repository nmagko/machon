#!/usr/bin/perl -w
#
# MACHine MONitor ANALyzer
# Copyright (C) 2007,2012  Victor C Salas Pumacayo (aka nmag) <nmagko@gmail.com>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

use strict "vars", "refs";

# Plotting code
my $plotter = <<__EXTN_PLOTTER;
#!/usr/bin/gnuplot -persist
set xdata time
set timefmt "%H:%M"
set format x "%H:%M"
set title "[Timeout: <ERRORS>] <OUTPUT><DETAIL>"
set xlabel "Time"
set yrange [-25:1360]
set xrange ["00:00":"23:59"]
set ylabel "Delays (ms)"
plot "<OUTPUT>.dat" using 1:2 index 1:1 \\
title "TOP" with boxes lt 2, \\
"<OUTPUT>.dat" using 1:2 index 0:0 \\
title "AVG.LOST" with boxes lt 1
__EXTN_PLOTTER

# Marks
#my $valuegraph_timeout = 2000;
#my $valuegraph_trunk = 1975; #set yrange [-25:2025]
my $valuegraph_timeout = 1256;
my $valuegraph_trunk = 1200;
my $valuetop = 3001;

# Main code
main ( my $argc=$#ARGV, my @argv=@ARGV ); sub main {
  my ($name, $hou, $min, $err, $qos, $lat);
  my $anal = {}; # Global N-Dimentional Database
  my %index = ( 'global', 1 ); # Basic index for making graphics
  my $ruler = {}; # 2-Dimentional index Database
  my %idxruler = (); # Basic index for steps in ruler
  my $byhour = -1;
  my $regdate = "";

  if ( $argc < 0 || $argc > 1 ) {
    printf("usar: %s <log_file> [hour]\n", $0);
    exit (1);
  }
  if ( defined $argv[1] && ( $argv[1] !~ /^\d+$/ || $argv[1] > 23 || $argv[1] < 0 ) ) {
    printf("usar: %s <log_file> [hour]\n", $0);
    exit (1);
  }
  $byhour = $argv[1] unless ( ! defined $argv[1] );
  $regdate = $argv[0];
  $regdate =~ s/\..+$//;
  $regdate =~ s/[^\-]+-//;
  if ( ! open ( logF, $argv[0] ) ) {
    printf("%s: The %s file can't be opened.\n", $0, $argv[0]);
    exit (1);
  }

  # Big Log Analyzer in n-order
  printf("%s: Processing %s log, wait...\n", $0, $regdate);
  foreach my $reg (<logF>) {
    $reg =~ s/^\s+||\s+$//g;
    ($name, $hou, $min, $err, $qos, $lat) = split (/,/, $reg);
    $ruler->{$hou}{$min} = 1;
    $idxruler{$hou} = 1;
    $index{$name} = 1;
    # error counters
    $anal->{$name}{err} = 0 unless ( defined $anal->{$name}{err} );
    $anal->{$name}{$hou}->{err} = 0 unless ( defined $anal->{$name}{$hou}->{err} );
    $anal->{global}{err} = 0 unless ( defined $anal->{global}{err} );
    $anal->{global}{$hou}->{err} = 0 unless ( defined $anal->{global}{$hou}->{err} );
    $anal->{global}{$hou}->{$min}{err} = 1 unless ( defined $anal->{global}{$hou}->{$min}{err} );
    $anal->{global}{$hou}->{$min}{ner} = 1 unless ( defined $anal->{global}{$hou}->{$min}{ner} );
    if ( ! ( $lat =~ /^\d+(\.\d+)?$/ ) ) {
      $anal->{$name}{err}++;
      $anal->{$name}{$hou}->{err}++;
      $anal->{global}{err}++;
      $anal->{global}{$hou}->{err}++;
      $lat = $valuetop;
      $anal->{$name}{$hou}->{$min}{yerr} = $valuegraph_timeout;
      $anal->{global}{$hou}->{$min}{yerr} += $valuegraph_timeout;
      $anal->{global}{$hou}->{$min}{err}++;
    } else {
      do {
        # $anal->{$name}{$hou}->{$min}{yerr} = 0;
        if ( ! defined $anal->{$name}{$hou}->{$min}{yerr} ) {
	  $anal->{$name}{$hou}->{$min}{yerr} = $lat;
	} else {
	  $anal->{$name}{$hou}->{$min}{yerr} = $lat unless $anal->{$name}{$hou}->{$min}{yerr} > $lat;
	}
        #*#$anal->{global}{$hou}->{$min}{yerr} = 0;
      } unless ( defined ( $anal->{$name}{$hou}->{$min}{yerr} ) );
      $anal->{global}{$hou}->{$min}{yerr} += $lat;
      $anal->{global}{$hou}->{$min}{ner}++;
    }
    # specific items
    $anal->{$name}{minqos} = 9999999999 unless ( defined $anal->{$name}{minqos} );
    if ( $anal->{$name}{minqos} > $qos ) { $anal->{$name}{minqos} = $qos; }
    if ( ! defined $anal->{$name}{qos} ) {
      $anal->{$name}{qos} = $qos;
      $anal->{$name}{qoscnt} = 1;
    } else {
      $anal->{$name}{qos} += $qos;
      $anal->{$name}{qoscnt}++;
    }
    if ( ! defined $anal->{$name}{lat} ) {
      $anal->{$name}{lat} = $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{$name}{latcnt} = 1 unless ( $lat !~ /^\d+(\.\d+)?$/ );
    } else {
      $anal->{$name}{lat} += $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{$name}{latcnt}++ unless ( $lat !~ /^\d+(\.\d+)?$/ );
    }
    if ( ! defined $anal->{$name}{$hou}->{$min}{lat} ) {
      $anal->{$name}{$hou}->{$min}{lat} = $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{$name}{$hou}->{$min}{latcnt} = 1 unless ( $lat !~ /^\d+(\.\d+)?$/ );
    } else {
      $anal->{$name}{$hou}->{$min}{lat} += $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{$name}{$hou}->{$min}{latcnt}++ unless ( $lat !~ /^\d+(\.\d+)?$/ );
    }
    # global result
    $anal->{global}{minqos} = 9999999999 unless ( defined $anal->{global}{minqos} );
    if ( $anal->{global}{minqos} > $qos ) { $anal->{global}{minqos} = $qos; }
    if ( ! defined $anal->{global}{lat} ) {
      $anal->{global}{lat} = $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{global}{latcnt} = 1 unless ( $lat !~ /^\d+(\.\d+)?$/ );
    } else {
      $anal->{global}{lat} += $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{global}{latcnt}++ unless ( $lat !~ /^\d+(\.\d+)?$/ );
    }
    if ( ! defined $anal->{global}{$hou}->{$min}{lat} ) {
      $anal->{global}{$hou}->{$min}{lat} = $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{global}{$hou}->{$min}{latcnt} = 1 unless ( $lat !~ /^\d+(\.\d+)?$/ );
    } else {
      $anal->{global}{$hou}->{$min}{lat} += $lat unless ( $lat !~ /^\d+(\.\d+)?$/ );
      $anal->{global}{$hou}->{$min}{latcnt}++ unless ( $lat !~ /^\d+(\.\d+)?$/ );
    }
  }
  close(logF);

  # Global QoS calculation
  foreach my $outputname (keys %index) {
    if ( $outputname ne "global" ) {
      if ( ! defined $anal->{global}{qos} ) {
        $anal->{$outputname}{qos} /= $anal->{$outputname}{qoscnt};
        $anal->{global}{qos} = $anal->{$outputname}{qos};
        $anal->{global}{qoscnt} = 1;
      } else {
        $anal->{$outputname}{qos} /= $anal->{$outputname}{qoscnt};
        $anal->{global}{qos} += $anal->{$outputname}{qos};
        $anal->{global}{qoscnt}++;
      }
    }
  }
  $anal->{global}{qos} /= $anal->{global}{qoscnt};

  printf("%s: Dumping processed data...\n", $0);
  # Make Saved File System
  `rm -Rf machon.saved` if (-d "machon.saved");
  if ( ! mkdir ("machon.saved", 0755) ) {
    printf("%s: The machon.saved directory can't be created.\n", $0);
    exit (1);
  }

  # Plotting Image in persistent medium
  foreach my $outputname (keys %index) {
    my ($ax, $ay, $execplot);
    my $tyerr = "";
    my $detail = "";
    my $secnblk = "\n\n";
    my $sizesbl = length($secnblk);
    if ( ! open (DATFILE, ">machon.saved/$outputname.dat") ) {
      printf("%s: The %s.dat file can't be created.\n", $0, $outputname);
      exit (1);
    }
    if ( $byhour < 0 ) {
      my $lasthour = "";
      foreach my $hour (sort keys %idxruler) {
        foreach my $mins (sort keys %{ $ruler->{$hour} }) {
          $ax = $hour . ":" . $mins;
          if ( defined $anal->{$outputname}{$hour}->{$mins}{lat} ) {
            $ay = $anal->{$outputname}{$hour}->{$mins}{lat} / $anal->{$outputname}{$hour}->{$mins}{latcnt};
            if ( $ay >  $valuegraph_trunk ) { $ay = $valuegraph_trunk };
            print(DATFILE sprintf("%s  %f\n", $ax, $ay));
          }
          do {
	      if ( $outputname eq "global" ) {
		  $secnblk .= sprintf("%s  %f\n", $ax, ( ( $anal->{$outputname}{$hour}->{$mins}{yerr} * $anal->{$outputname}{$hour}->{$mins}{err} ) / ( $anal->{$outputname}{$hour}->{$mins}{ner} + $anal->{$outputname}{$hour}->{$mins}{err} - 1 ) ));
	      } else {
		  $secnblk .= sprintf("%s  %f\n", $ax, $anal->{$outputname}{$hour}->{$mins}{yerr});
	      }
          } unless ( ! defined $anal->{$outputname}{$hour}->{$mins}{yerr} );
        }
        $lasthour = $hour;
      }
      $secnblk .= $lasthour . ":00  3100.000000\n" if ( $sizesbl == length($secnblk) );
    } else {
      foreach my $mins (sort keys %{ $ruler->{$byhour} }) {
        $ax = $byhour . ":" . $mins;
        if ( defined $anal->{$outputname}{$byhour}->{$mins}{lat} ) {
          $ay = $anal->{$outputname}{$byhour}->{$mins}{lat} / $anal->{$outputname}{$byhour}->{$mins}{latcnt};
          if ( $ay >  $valuegraph_trunk ) { $ay = $valuegraph_trunk };
          print(DATFILE sprintf("%s  %f\n", $ax, $ay));
        }
        do {
	      if ( $outputname eq "global" ) {
		  $secnblk .= sprintf("%s  %f\n", $ax, ( ( $anal->{$outputname}{$byhour}->{$mins}{yerr} * $anal->{$outputname}{$byhour}->{$mins}{err} ) / ( $anal->{$outputname}{$byhour}->{$mins}{ner} + $anal->{$outputname}{$byhour}->{$mins}{err} - 1 ) ));
	      } else {
		  $secnblk .= sprintf("%s  %f\n", $ax, $anal->{$outputname}{$byhour}->{$mins}{yerr});
	      }
        } unless ( ! defined $anal->{$outputname}{$byhour}->{$mins}{yerr} );
      }
      $secnblk .= $byhour . ":00  3100.000000\n" if ( $sizesbl == length($secnblk) );
    }
    print(DATFILE $secnblk);
    close (DATFILE);
    if ( ! open (EXEFILE, ">machon.saved/$outputname.plt") ) {
      printf("%s: The %s.plt executable file can't be created.\n", $0, $outputname);
      exit (1);
    }
    $execplot = $plotter;

    if ( $byhour < 0 ) {
	$tyerr = $anal->{$outputname}{err};
    } else {
	$tyerr = $anal->{$outputname}{$byhour}->{err};
    }
    $detail = " (lp:" . sprintf("%.2f", ($anal->{$outputname}{lat}/$anal->{$outputname}{latcnt})) .
              ", qosm:" . $anal->{$outputname}{minqos} .
              ", qos:" . sprintf("%.2f", $anal->{$outputname}{qos}) . ") " .
              $regdate if ( $byhour < 0 );
    $detail = " (" . $byhour . ":00-" . $byhour . ":59) " . $regdate if ( $byhour >= 0 );
    $execplot =~ s/<DETAIL>/$detail/;
    $execplot =~ s/<OUTPUT>/$outputname/g;
    $execplot =~ s/<ERRORS>/$tyerr/;
    do {
	$execplot =~ s/\"00:/\"$byhour:/;
	$execplot =~ s/\"23:/\"$byhour:/;
    } unless ( $byhour < 0 );
    print(EXEFILE $execplot);
    close (EXEFILE);
    chmod (0755, "machon.saved/$outputname.plt");
  }
  printf("%s: done.\n", $0);
  exit (0);
}

__END__
