# insert tail calls into C#/ildasm
# print out the original and generate separate files

use warnings FATAL => 'all';

# globals
my $methods = { };
my @checks;

sub merge_lines_call
{
	my $lines = shift;
	my @out;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];

		# Remove these:
		if ($line =~ /permissionset
			| PermissionAttribute
			| \.custom /ix
			|| $line =~ /^\s*[0-9A-F]{2}\s+/i
			|| $line =~ /^\s*=/
			)
		{
			next;
		}

		# Join call lines, though keeping newlines.

		if ($line =~ / : \s* call.+\(/x)
		{
			# read the entire signature
			while ($line !~ /\)\n$/)
			{
				$line .= $lines->[++$i];
			}
		}

		push(@out, $line);

	}
	return \@out;
}

sub merge_lines_all
{
	my $lines = shift;
	my @out;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];

		# Join .method lines.
		# Joine .local ilnes.

		if ($line =~ /\.locals init \(/)
		{
			while ($line !~ /\)\n$/)
			{
				$line .= $lines->[++$i];
			}
			$line =~ s/\s+/ /g;
			$line .= "\n";
		}
		elsif ($line =~ /\.method/)
		{
			while ($line !~ /{/)
			{
				$line .= $lines->[++$i];
			}
			$line =~ s/\s+/ /g;
			$line .= "\n";
		}

		push(@out, $line);

	}
	return \@out;
}

sub add_tails
{
	my $lines = shift;
	my @out;
	my %calls;
	my $skipret = 0;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];
		if ($skipret)
		{
			$skipret = 0;
			next if $line =~ / ret$/;
		}
		if ($line =~ / : \s* call (?:virt)? \s+ (?:instance\s+)? int32 \s+ A:: ([a-z]*?tail(.?) 2 [^(]* )\(/xi)
		{
			# $1 is the name
			# $2 is if the name has 'i' before the 2

			my $i = $2; # taili => calli
			if (++$calls{$1} == 2) # put tail on second call to foo2
			{
				# insert tail
				$line =~ s/\scall/ tail. call/;

				# if it is "taili" then turn into ldftn and calli
				# later maybe: virtual and non-static, but at least one calli case
				if ($i)
				{
					$line =~ / ( int32 \s+ A:: [^(]+ ( \( [^)]+ \) )) /xi;
					# $1 is the entire method signature with name for ldftn
					# $2 is the signature without the name (or return) for calli
					push(@out, "ldftn $1\n");
					push(@out, "tail. calli int32 $2\n");
				}
				else
				{
					push(@out, $line);
				}
				$skipret = 1;
				$line = "ret\n";
			}
		}

		push(@out, $line);
	}
	return \@out;
}

sub split_methods
{
	my $lines = shift;
	my @out;
	my $body;
	my $check;
	my $sig;
	my $labels;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];

		$line =~ s/\/\/.*$//; # remove comments
		$line =~ s/\s+/ /g;
		$line =~ s/^\s+/ /;
		$line =~ s/ +$//;
		$line .= "\n";

		# Move .method lines into per-method hashes
		# keyed by signature. Signature starts with the token before the paren
		# and extends to the closing paren.
		#
		# Eventual goal is to provide a source file per test, where test
		# is renamed to main, and reusable "check" functions are present.
		#
		# Later we might want to rewrite this using reflection / C# / Cecil.

		if ($line =~ /\.method/)
		{
			$line =~ / ([a-z0-9.<>]+\([^)]*\)) /i;
			$sig = $1 || die("sig not found $line");
			$sig =~ /([a-z0-9.]+)/i;
			my $name = $1;
			$body = [ ];
			my $method = { body => $body };
			$labels = { };
			$methods->{$name} = $method;
			$method->{body} = $body;
			$method->{name} = $name;
			$method->{sig} = $sig;
			$method->{labels} = $labels;
			if ($name eq "check")
			{
				push(@checks, $method)
			}
		}

		push(@out, $line);

		if ($body)
		{
			push(@{$body}, $line);
		}
		if ($labels && $line =~ / (IL_[0-9a-f]{4})$/)
		{
			$labels->{$1} = 1;
		}
		if ($line =~ /}$/)
		{
			undef $sig;
			undef $labels;
			undef $check;
			undef $body;
		}
	}
	return \@out;
}

sub remove_unused_labels_one
{
	my $sig = shift; # for debug
	my $lines = shift;
	my $labels = shift;
	my @out;

	for (my $i = 0; $i < scalar @{$lines}; ++$i)
	{
		my $line = $lines->[$i];
		chomp($line);
		$line =~ s/\s+$//;
		$line =~ s/^\s+//;

		if ($line =~ /^(IL_[0-9a-f]{4}): (.+)/)
		{
			if (!$labels->{$1})
			{
				$line = $2;
			}
		}
		next if $line eq "";
		push(@out, "$line\n");
	}

	return \@out;
}

sub remove_unused_lables_all
{
	for my $method (values %{$methods})
	{
		$method->{body} = remove_unused_labels_one($method->{sig}, $method->{body}, $method->{labels});
	}
	for my $method (@checks)
	{
		$method->{body} = remove_unused_labels_one($method->{sig}, $method->{body}, $method->{labels});
	}
}

sub separate_print
{
	my $checktext;
	for my $check (@checks)
	{
		$checktext .= join("", @{$check->{body}});
	}
	#die($checktext);
	for my $method (values %{$methods})
	{
		my $sig = $method->{sig};
		my $name = $method->{name};
		if ($name =~ /1$/)
		{
			# open per-test case file
			open (my $f, ">", $method->{name} . ".il") || die;

			my $body = join("", @{$method->{body}});

			# print prefix
			print $f <<End;
.assembly extern mscorlib { }
.assembly tailcall1 { }
.class A extends [mscorlib]System.Object {
End
			print $f <<End if $body =~ /newobj instance void A::\.ctor\(\)/;
.method public hidebysig specialname rtspecialname instance void .ctor() cil managed
{
.maxstack 8
ldarg.0
call instance void [mscorlib]System.Object::.ctor()
ret
}
End
			$body =~ s/\{/\{\n.entrypoint/;
			$body =~ s/\n+/\n/mg;
			print($f $body);
			$name =~ s/1$/2/;
			print($f @{$methods->{$name}->{body}});
			# print suffix
			print($f $checktext);
			print($f "}\n");
		}
	}
}

my @lines = <>;
my $lines = merge_lines_call(\@lines);
$lines = add_tails($lines);
print(@{$lines});
$lines = merge_lines_all($lines);
$lines = split_methods($lines);
remove_unused_lables_all();
separate_print();
