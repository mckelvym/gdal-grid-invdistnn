%perlcode %{
    use strict;
    use Carp;
    use Geo::GDAL::Const;
    use Geo::OGR;
    use Geo::OSR;
    our $VERSION = '0.21';
    use vars qw/
	%TYPE_STRING2INT %TYPE_INT2STRING
	%ACCESS_STRING2INT %ACCESS_INT2STRING
	%RESAMPLING_STRING2INT %RESAMPLING_INT2STRING
	%NODE_TYPE_STRING2INT %NODE_TYPE_INT2STRING
	/;
    for my $string (qw/Unknown Byte UInt16 Int16 UInt32 Int32 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64/) {
	my $int = eval "\$Geo::GDAL::Constc::GDT_$string";
	$TYPE_STRING2INT{$string} = $int;
	$TYPE_INT2STRING{$int} = $string;
    }
    for my $string (qw/ReadOnly Update/) {
	my $int = eval "\$Geo::GDAL::Constc::GA_$string";
	$ACCESS_STRING2INT{$string} = $int;
	$ACCESS_INT2STRING{$int} = $string;
    }
    for my $string (qw/NearestNeighbour Bilinear Cubic CubicSpline/) {
	my $int = eval "\$Geo::GDAL::Constc::GRA_$string";
	$RESAMPLING_STRING2INT{$string} = $int;
	$RESAMPLING_INT2STRING{$int} = $string;
    }
    {
	my $int = 0;
	for my $string (qw/Element Text Attribute Comment Literal/) {
	    $NODE_TYPE_STRING2INT{$string} = $int;
	    $NODE_TYPE_INT2STRING{$int} = $string;
	    $int++;
	}
    }
    sub RELEASE_PARENTS {
    }
    sub NodeType {
	my $type = shift;
	return $NODE_TYPE_INT2STRING{$type} if $type =~ /^\d/;
	return $NODE_TYPE_STRING2INT{$type};
    }
    sub NodeData {
	my $node = shift;
	return (Geo::GDAL::NodeType($node->[0]), $node->[1]);
    }
    sub Children {
	my $node = shift;
	return @$node[2..$#$node];
    }
    sub Child {
	my($node, $child) = @_;
	return $node->[2+$child];
    }
    sub GetDataTypeSize {
	my $t = shift;
	$t = $TYPE_INT2STRING{$t} if exists $TYPE_INT2STRING{$t};
	return _GetDataTypeSize($t);
    }
    sub DataTypeIsComplex {
	my $t = shift;
	$t = $TYPE_INT2STRING{$t} if exists $TYPE_INT2STRING{$t};
	return _DataTypeIsComplex($t);
    }
    sub PackCharacter {
	my $t = shift;
	$t = $TYPE_INT2STRING{$t} if exists $TYPE_INT2STRING{$t};
	my $is_big_endian = unpack("h*", pack("s", 1)) =~ /01/; # from Programming Perl
	return 'C' if $t =~ /^Byte$/;
	return ($is_big_endian ? 'n': 'v') if $t =~ /^UInt16$/;
	return 's' if $t =~ /^Int16$/;
	return ($is_big_endian ? 'N' : 'V') if $t =~ /^UInt32$/;
	return 'l' if $t =~ /^Int32$/;
	return 'f' if $t =~ /^Float32$/;
	return 'd' if $t =~ /^Float64$/;
	croak "unsupported data type: $t";
    }
    sub Drivers {
	my @drivers;
	for my $i (0..GetDriverCount()-1) {
	    push @drivers, _GetDriver($i);
	}
	return @drivers;
    }
    sub GetDriver {
	my $driver = shift;
	return _GetDriver($driver) if $driver =~ /^\d/;
	return GetDriverByName($driver);
    }
    sub Open {
	my @p = @_;
	$p[1] = $ACCESS_STRING2INT{$p[1]} if $p[1] and exists $ACCESS_STRING2INT{$p[1]};
	return _Open(@p);
    }
    sub OpenShared {
	my @p = @_;
	$p[1] = $ACCESS_STRING2INT{$p[1]} if $p[1] and exists $ACCESS_STRING2INT{$p[1]};
	return _OpenShared(@p);
    }
    sub ComputeMedianCutPCT {
	my($red, $green, $blue, $num_colors, $colors, $callback, $callback_data) = @_;
	_ComputeMedianCutPCT($red, $green, $blue, $num_colors, $colors, $callback, $callback_data);
    }
    sub DitherRGB2PCT {
	my($red, $green, $blue, $target, $colors, $callback, $callback_data) = @_;
	_DitherRGB2PCT($red, $green, $blue, $target, $colors, $callback, $callback_data);
    }
    sub ReprojectImage {
	my @p = @_;
	$p[4] = $RESAMPLING_STRING2INT{$p[4]} if $p[4] and exists $RESAMPLING_STRING2INT{$p[4]};
	return _ReprojectImage(@p);
    }
    sub AutoCreateWarpedVRT {
	my @p = @_;
	$p[3] = $RESAMPLING_STRING2INT{$p[3]} if $p[3] and exists $RESAMPLING_STRING2INT{$p[3]};
	return _AutoCreateWarpedVRT(@p);
    }
    package Geo::GDAL::MajorObject;
    use vars qw/@DOMAINS/;
    use strict;
    sub Domains {
	return @DOMAINS;
    }
    sub Description {
	my($self, $desc) = @_;
	SetDescription($self, $desc) if defined $desc;
	GetDescription($self) if defined wantarray;
    }
    sub Metadata {
	my $self = shift;
	my $metadata = shift if ref $_[0];
	my $domain = shift;
	$domain = '' unless defined $domain;
	SetMetadata($self, $metadata, $domain) if defined $metadata;
	GetMetadata($self, $domain) if defined wantarray;
    }
    package Geo::GDAL::Driver;
    use vars qw/@CAPABILITIES @DOMAINS/;
    use strict;
    @CAPABILITIES = ('Create', 'CreateCopy');
    sub Domains {
	return @DOMAINS;
    }
    sub Name {
	my $self = shift;
	return $self->{ShortName};
    }
    sub Capabilities {
	my $self = shift;
	return @CAPABILITIES unless shift;
	my $h = $self->GetMetadata;
	my @cap;
	for my $cap (@CAPABILITIES) {
	    push @cap, $cap if $h->{'DCAP_'.uc($cap)} eq 'YES';
	}
	return @cap;
    }
    sub TestCapability {
	my($self, $cap) = @_;
	my $h = $self->GetMetadata;
	return $h->{'DCAP_'.uc($cap)} eq 'YES' ? 1 : undef;
    }
    sub Extension {
	my $self = shift;
	my $h = $self->GetMetadata;
	return $h->{DMD_EXTENSION};
    }
    sub MIMEType {
	my $self = shift;
	my $h = $self->GetMetadata;
	return $h->{DMD_MIMETYPE};
    }
    sub CreationOptionList {
	my $self = shift;
	my @options;
	my $h = $self->GetMetadata->{DMD_CREATIONOPTIONLIST};
	if ($h) {
	    $h = Geo::GDAL::ParseXMLString($h);
	    my($type, $value) = Geo::GDAL::NodeData($h);
	    if ($value eq 'CreationOptionList') {
		for my $o (Geo::GDAL::Children($h)) {
		    my %option;
		    for my $a (Geo::GDAL::Children($o)) {
			my(undef, $key) = Geo::GDAL::NodeData($a);
			my(undef, $value) = Geo::GDAL::NodeData(Geo::GDAL::Child($a, 0));
			if ($key eq 'Value') {
			    push @{$option{$key}}, $value;
			} else {
			    $option{$key} = $value;
			}
		    }
		    push @options, \%option;
		}
	    }
	}
	return @options;
    }
    sub CreationDataTypes {
	my $self = shift;
	my $h = $self->GetMetadata;
	return split /\s+/, $h->{DMD_CREATIONDATATYPES};
    }
    sub CreateDataset {
	my @p = @_;
	$p[5] = $Geo::GDAL::TYPE_STRING2INT{$p[5]} if $p[5] and exists $Geo::GDAL::TYPE_STRING2INT{$p[5]};
	return _Create(@p);
    }
    *Create = *CreateDataset;
    *CopyDataset = *CreateCopy;
    package Geo::GDAL::Dataset;
    use strict;
    use vars qw/%BANDS @DOMAINS/;
    @DOMAINS = ("IMAGE_STRUCTURE", "SUBDATASETS", "GEOLOCATION");
    sub Domains {
	return @DOMAINS;
    }
    sub Open {
	return Geo::GDAL::Open(@_);
    }
    sub OpenShared {
	return Geo::GDAL::OpenShared(@_);
    }
    sub Size {
	my $self = shift;
	return ($self->{RasterXSize}, $self->{RasterYSize});
    }
    sub Bands {
	my $self = shift;
	my @bands;
	for my $i (1..$self->{RasterCount}) {
	    push @bands, GetRasterBand($self, $i);
	}
	return @bands;
    }
    sub GetRasterBand {
	my($self, $index) = @_;
	my $band = _GetRasterBand($self, $index);
	$BANDS{tied(%{$band})} = $self;
	return $band;
    }
    *Band = *GetRasterBand;
    sub AddBand {
	my @p = @_;
	$p[1] = $Geo::GDAL::TYPE_STRING2INT{$p[1]} if $p[1] and exists $Geo::GDAL::TYPE_STRING2INT{$p[1]};
	return _AddBand(@p);
    }
    sub Projection {
	my($self, $proj) = @_;
	SetProjection($self, $proj) if defined $proj;
	GetProjection($self) if defined wantarray;
    }
    sub GeoTransform {
	my $self = shift;
	SetGeoTransform($self, \@_) if @_ > 0;
	return unless defined wantarray;
	my $t = GetGeoTransform($self);
	return @$t;
    }
    sub GCPs {
	my $self = shift;
	if (@_ > 0) {
	    my $proj = pop @_;
	    SetGCPs($self, \@_, $proj);
	}
	return unless defined wantarray;
	my $proj = GetGCPProjection($self);
	my $GCPs = GetGCPs($self);
	return (@$GCPs, $proj);
    }
    package Geo::GDAL::Band;
    use Carp;
    use UNIVERSAL qw(isa);
    use strict;
    use vars qw/
	%COLOR_INTERPRETATION_STRING2INT %COLOR_INTERPRETATION_INT2STRING @DOMAINS
	/;
    for my $string (qw/Undefined GrayIndex PaletteIndex RedBand GreenBand BlueBand AlphaBand 
		    HueBand SaturationBand LightnessBand CyanBand MagentaBand YellowBand BlackBand/) {
	my $int = eval "\$Geo::GDAL::Constc::GCI_$string";
	$COLOR_INTERPRETATION_STRING2INT{$string} = $int;
	$COLOR_INTERPRETATION_INT2STRING{$int} = $string;
    }
    @DOMAINS = ("IMAGE_STRUCTURE", "RESAMPLING");
    sub Domains {
	return @DOMAINS;
    }
    sub DESTROY {
	my $self;
	if ($_[0]->isa('SCALAR')) {
	    $self = $_[0];
	} else {
	    return unless $_[0]->isa('HASH');
	    $self = tied(%{$_[0]});
	    return unless defined $self;
	}
	delete $ITERATORS{$self};
	if (exists $OWNER{$self}) {
	    delete $OWNER{$self};
	}
	$self->RELEASE_PARENTS();
    }
    sub RELEASE_PARENTS {
	my $self = shift;
	delete $Geo::GDAL::Dataset::BANDS{$self};
    }
    sub Size {
	my $self = shift;
	return ($self->{XSize}, $self->{YSize});
    }
    sub DataType {
	my $self = shift;
	return $Geo::GDAL::TYPE_INT2STRING{$self->{DataType}};
    }
    sub NoDataValue {
	my $self = shift;
	SetNoDataValue($self, $_[0]) if @_ > 0;
	GetNoDataValue($self);
    }
    sub ReadTile {
	my($self, $xoff, $yoff, $xsize, $ysize) = @_;
	$xoff = 0 unless defined $xoff;
	$yoff = 0 unless defined $yoff;
	$xsize = $self->{XSize} - $xoff unless defined $xsize;
	$ysize = $self->{YSize} - $yoff unless defined $ysize;
	my $buf = $self->ReadRaster($xoff, $yoff, $xsize, $ysize);
	my $pc = Geo::GDAL::PackCharacter($self->{DataType});
	my $w = $xsize * Geo::GDAL::GetDataTypeSize($self->{DataType})/8;
	my $offset = 0;
	my @data;
	for (0..$ysize-1) {
	    my $sub = substr($buf, $offset, $w);
	    my @d = unpack($pc."[$xsize]", $sub);
	    push @data, \@d;
	    $offset += $w;
	}
	return \@data;
    }
    sub WriteTile {
	my($self, $data, $xoff, $yoff) = @_;
	$xoff = 0 unless defined $xoff;
	$yoff = 0 unless defined $yoff;
	my $xsize = @{$data->[0]};
	$xsize = $self->{XSize} - $xoff if $xsize > $self->{XSize} - $xoff;
	my $ysize = @{$data};
	$ysize = $self->{YSize} - $yoff if $ysize > $self->{YSize} - $yoff;
	my $pc = Geo::GDAL::PackCharacter($self->{DataType});
	my $w = $xsize * Geo::GDAL::GetDataTypeSize($self->{DataType})/8;
	for my $i (0..$ysize-1) {
	    my $scanline = pack($pc."[$xsize]", @{$data->[$i]});
	    $self->WriteRaster( $xoff, $yoff+$i, $xsize, 1, $scanline );
	}
    }
    sub ColorInterpretation {
	my($self, $ci) = @_;
	if ($ci) {
	    $ci = $COLOR_INTERPRETATION_STRING2INT{$ci} if exists $COLOR_INTERPRETATION_STRING2INT{$ci};
	    SetRasterColorInterpretation($self, $ci);
	    return $ci;
	} else {
	    return $COLOR_INTERPRETATION_INT2STRING{GetRasterColorInterpretation($self)};
	}
    }
    sub ColorTable {
	my $self = shift;
	SetRasterColorTable($self, $_[0]) if @_;
	return unless defined wantarray;
	GetRasterColorTable($self);
    }
    sub CategoryNames {
	my $self = shift;
	SetRasterCategoryNames($self, \@_) if @_;
	return unless defined wantarray;
	my $n = GetRasterCategoryNames($self);
	return @$n;
    }
    sub AttributeTable {
	my $self = shift;
	SetDefaultRAT($self, $_[0]) if @_;
	return unless defined wantarray;
	my $r = GetDefaultRAT($self);
	$Geo::GDAL::RasterAttributeTable::BANDS{$r} = $self;
	return $r;
    }
    sub Contours {
	my $self = shift;
	my %defaults = (DataSource => undef,
			LayerConstructor => {Name => 'contours'},
			ContourInterval => 100, 
			ContourBase => 0,
			FixedLevels => [], 
			NoDataValue => undef, 
			IDField => -1, 
			ElevField => -1,
			callback => undef,
			callback_data => undef);
	my %params;
	if (!defined($_[0]) or isa($_[0], 'Geo::OGR::DataSource')) {
	    ($params{DataSource}, $params{LayerConstructor},
	     $params{ContourInterval}, $params{ContourBase},
	     $params{FixedLevels}, $params{NoDataValue}, 
	     $params{IDField}, $params{ElevField},
	     $params{callback}, $params{callback_data}) = @_;
	} else {
	    %params = @_;
	}
	for (keys %params) {
	    croak "unknown parameter: $_" unless exists $defaults{$_};
	}
	for (keys %defaults) {
	    $params{$_} = $defaults{$_} unless defined $params{$_};
	}
	$params{DataSource} = Geo::OGR::GetDriver('Memory')->CreateDataSource('ds') 
	    unless defined $params{DataSource};
	my $layer = $params{DataSource}->CreateLayer($params{LayerConstructor});
	my $schema = $layer->GetLayerDefn;
	for ('IDField', 'ElevField') {
	    $params{$_} = $schema->GetFieldIndex($params{ElevField}) unless $params{ElevField} =~ /^[+-]?\d+$/;
	}
	ContourGenerate($self, $params{ContourInterval}, $params{ContourBase}, $params{FixedLevels},
			$params{NoDataValue}, $layer, $params{IDField}, $params{ElevField},
			$params{callback}, $params{callback_data});
	return $layer;
    }
    package Geo::GDAL::ColorTable;
    use strict;
    use vars qw/
	%PALETTE_INTERPRETATION_STRING2INT %PALETTE_INTERPRETATION_INT2STRING
	/;
    for my $string (qw/Gray RGB CMYK HLS/) {
	my $int = eval "\$Geo::GDAL::Constc::GPI_$string";
	$PALETTE_INTERPRETATION_STRING2INT{$string} = $int;
	$PALETTE_INTERPRETATION_INT2STRING{$int} = $string;
    }
    sub create {
	my($pkg, $pi) = @_;
	$pi = $PALETTE_INTERPRETATION_STRING2INT{$pi} if defined $pi and exists $PALETTE_INTERPRETATION_STRING2INT{$pi};
	my $self = Geo::GDALc::new_ColorTable($pi);
	bless $self, $pkg if defined($self);
    }
    sub GetPaletteInterpretation {
	my $self = shift;
	return $PALETTE_INTERPRETATION_INT2STRING{GetPaletteInterpretation($self)};
    }
    sub SetColorEntry {
	@_ == 3 ? _SetColorEntry(@_) : _SetColorEntry(@_[0..1], [@_[2..5]]);
    }
    sub ColorEntry {
	my $self = shift;
	my $index = shift;
	SetColorEntry($self, $index, @_) if @_ > 0;
	GetColorEntry($self, $index) if defined wantarray;
    }
    sub ColorTable {
	my $self = shift;
	my @table;
	if (@_) {
	    my $index = 0;
	    for my $color (@_) {
		push @table, [ColorEntry($self, $index, @$color)];
		$index++;
	    }
	} else {
	    for (my $index = 0; $index < GetCount($self); $index++) {
		push @table, [ColorEntry($self, $index)];
	    }
	}
	return @table;
    }
    package Geo::GDAL::RasterAttributeTable;
    use strict;
    use vars qw/ %BANDS
	%FIELD_TYPE_STRING2INT %FIELD_TYPE_INT2STRING
	%FIELD_USAGE_STRING2INT %FIELD_USAGE_INT2STRING
	/;
    for my $string (qw/Integer Real String/) {
	my $int = eval "\$Geo::GDAL::Constc::GFT_$string";
	$FIELD_TYPE_STRING2INT{$string} = $int;
	$FIELD_TYPE_INT2STRING{$int} = $string;
    }
    for my $string (qw/Generic PixelCount Name Min Max MinMax 
		    Red Green Blue Alpha RedMin 
		    GreenMin BlueMin AlphaMin RedMax GreenMax BlueMax AlphaMax 
		    MaxCount/) {
	my $int = eval "\$Geo::GDAL::Constc::GFU_$string";
	$FIELD_USAGE_STRING2INT{$string} = $int;
	$FIELD_USAGE_INT2STRING{$int} = $string;
    }
    sub FieldTypes {
	return keys %FIELD_TYPE_STRING2INT;
    }
    sub FieldUsages {
	return keys %FIELD_USAGE_STRING2INT;
    }
    sub RELEASE_PARENTS {
	my $self = shift;
	delete $BANDS{$self};
    }
    sub GetUsageOfCol {
	my($self, $col) = @_;
	$FIELD_USAGE_INT2STRING{_GetUsageOfCol($self, $col)};
    }
    sub GetColOfUsage {
	my($self, $usage) = @_;
	_GetColOfUsage($self, $FIELD_USAGE_STRING2INT{$usage});
    }
    sub GetTypeOfCol {
	my($self, $col) = @_;
	$FIELD_TYPE_INT2STRING{_GetTypeOfCol($self, $col)};
    }
    sub CreateColumn {
	my($self, $name, $type, $usage) = @_;
	_CreateColumn($self, $name, $FIELD_TYPE_STRING2INT{$type}, $FIELD_USAGE_STRING2INT{$usage});
    }
    sub Value {
	my($self, $row, $column) = @_;
	SetValueAsString($self, $row, $column, $_[3]) if defined $_[3];
	return unless defined wantarray;
	GetValueAsString($self, $row, $column);
    }

 %}

%extend GDALRasterBandShadow {
    %apply (int nList, double* pList) {(int nFixedLevelCount, double *padfFixedLevels)};
    %apply (int defined, double value) {(int bUseNoData, double dfNoDataValue)};
    CPLErr ContourGenerate(double dfContourInterval, double dfContourBase,
			   int nFixedLevelCount, double *padfFixedLevels,
			   int bUseNoData, double dfNoDataValue, 
			   OGRLayerShadow *hLayer, int iIDField, int iElevField,
			   GDALProgressFunc callback,
			   void* callback_data) {
	return GDALContourGenerate( self, dfContourInterval, dfContourBase,
				    nFixedLevelCount, padfFixedLevels,
				    bUseNoData, dfNoDataValue, 
				    hLayer, iIDField, iElevField,
				    callback,
				    callback_data );
    }
    %clear (int nFixedLevelCount, double *padfFixedLevels);
    %clear (int bUseNoData, double dfNoDataValue);
}
