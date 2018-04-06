use warnings FATAL => 'all';

# globals
my $methods;

sub cleanup_line
{
	my $line = shift;
	return $line;

	$line =~ s/\s+/ /g;
	$line =~ s/\s+$//;
	$line =~ s/\s+\[opt\]\s+/ /g;
	$line =~ s/ beforefieldinit / /g;
	$line =~ s/ rtspecialname / /;
	$line =~ s/ specialname / /;
	$line =~ s/ hidebysig / /;
	#$line =~ s/\[\[/\[ \]/;
	$line =~ s/ cil managed / /;
	$line =~ s/ cil managed$//;
	$line =~ s/ auto / /;
	$line =~ s/ ansi / /;
	$line =~ s/\)noinlining/\) noinlining/;
	$line =~ s/^\s*\.maxstack\s+[0-8]\s*$//;
	$line =~ s/extends\s+\[mscorlib\]System\.Object//;
	$line =~ s/^nop$//;
	$line =~ s/^\.maxstack 8$//;

	return $line;
}

sub merge_lines
{
	my $lines = shift;
	my @out;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];

		# Remove these:
		if ($line =~ /permissionset
			| PermissionAttribute
#			| \.custom 
			| \.ver 
			| \.publickey
			| \.file
			| \.image
			| \.sub
			| \.stack
			| \.param
			| \.corflags
			| \.hash
			| \.maxstack [0-8]$/ix
#			|| $line =~ /^\s*[0-9A-F]{2}\s+/i
#			|| $line =~ /^\s*=/
			)
		{
			next;
		}

		$line = cleanup_line($line);
		next if $line =~ /^\s*$/;
		next if $line =~ /^\s*nop$/;

		# Join call lines, though keeping newlines.

		if ($line =~ / : \s* call.+\(/x)
		{
			# read the entire signature
			while ($line !~ /\)/ && $i < scalar @{$lines})
			{
				$line .= $lines->[++$i] || "";	
				$line = cleanup_line($line);
			}
		}

		# Join .method lines.
		# Joine .local ilnes.

		elsif ($line =~ /\.locals init \(/)
		{
			while ($line !~ /\)/)
			{
				$line .= $lines->[++$i];
				$line = cleanup_line($line);
			}
			$line =~ s/\s+/ /g;
			$line .= "\n";
		}
		elsif ($line =~ /\.method/)
		{
			while ($line !~ /{/)
			{
				$line .= $lines->[++$i];
				$line = cleanup_line($line);
			}
			$line =~ s/\s+/ /g;
			$line .= "\n";
			$line =~ s/{\s*$//;
			$line =~ s/\s+$//;
			push(@out, $line);
			$line = "{\n";
		}

		$line =~ s/\s+$//;
		$line =~ s/^\s+//;

		push(@out, $line);

	}
	return \@out;
}

sub split_methods
{
	my $lines = shift;
	my $body;
	my $labels;
	my $before;
	my $after;
	my @method;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];

		$line =~ s/\/\/.*$//; # remove comments
		$line =~ s/\s+/ /g;
		$line =~ s/^\s+//;
		$line =~ s/ +$//;
		$line .= "\n";

		if ($line =~ /\.method/)
		{
			$body = [ ];
			$labels = { };
			$method = { body => $body, labels => $labels, before => $before };
			$before = [ ];
			$after = [ ];
			push(@{$methods}, $method);
		}

		#push(@out, $line);

		if ($body)
		{
			push(@{$body}, $line);
		}
		else
		{
			push(@{$before}, $line);
			push(@{$after}, $line);
		}
		if ($labels && $line =~ / (IL_[0-9a-f]{4})(\))?$/)
		{
			$labels->{$1} = 1;
		}
		elsif ($labels && $line =~ /^(IL_[0-9a-f]{4})([),])?$/)
		{
			$labels->{$1} = 1;
		}
		if ($method && $line =~ /}/)
		{
			undef $labels;
			undef $body;
		}
	}
	if ($method && $after)
	{
		$method->{after} = $after;
	}
}

sub remove_unused_labels_one
{
	my $lines = shift;
	my $labels = shift;
	my @out;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];
		chomp($line);
		$line =~ s/\s+$//;
		$line =~ s/^\s+//;
		if ($line =~ /^\s*(IL_[0-9a-f]{4}): (.+)/)
		{
			if ($labels->{$1})
			{
				push(@out, "$1:\n");
			}
			$line = $2;
		}
		next if $line eq "";
		next if $line eq "nop";
		push(@out, "$line\n");
	}

	return \@out;
}

sub remove_unused_lables_all
{
	for my $method (@{$methods})
	{
		$method->{body} = remove_unused_labels_one($method->{body}, $method->{labels});
	}
}

sub separate_print
{
	for my $method (@{$methods})
	{
		print join("", @{$method->{before}});
		print join("", @{$method->{body}});
		print join("", @{$method->{after}}) if $method->{after};
	}
}

my @lines = <>;
my $lines = merge_lines (\@lines);
#my $lines = \@lines;
#$lines = split_methods ($lines);
split_methods ($lines);
remove_unused_lables_all ();
#print(@{$lines});
separate_print ();
