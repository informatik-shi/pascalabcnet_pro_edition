unit ipaddress1;

interface

uses System, System.Net, System.Numerics, System.Collections.Generic, SPythonSystem;

type
  AddressValueError = class(ValueError);
  NetmaskValueError = class(ValueError);
  IPAddressValue = class;
  IPNetworkValue = class;

  IPPair = class(IReadOnlyCollection<IPAddressValue>)
  private
    first, second: IPAddressValue;
    function GetItem(index: integer): IPAddressValue;
    function GetCount(): integer;
  public
    constructor Create(first, second: IPAddressValue);
    property Count: integer read GetCount;
    property ByIndex[index: integer]: IPAddressValue read GetItem; default;
    function GetEnumerator(): IEnumerator<IPAddressValue>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function ToString(): string; override;
  end;

  IPMixedKey = class(IComparable<IPMixedKey>)
  private
    family: integer;
    number: BigInteger;
    mask: BigInteger;
    network: boolean;
    representation: string;
  public
    constructor Create(value: IPAddressValue);
    constructor Create(value: IPNetworkValue);
    function CompareTo(other: IPMixedKey): integer;
    function ToString(): string; override;
  end;

  IPAddressValue = class(IPythonIntConvertible, IComparable<IPAddressValue>)
  private
    number: BigInteger;
    family: integer;
    zone: string;
    function GetVersion(): integer;
    function GetMaxPrefixlen(): integer;
    function GetPacked(): bytes;
    function GetCompressed(): string;
    function GetExploded(): string; virtual;
    function GetReversePointer(): string;
    function GetIsPrivate(): boolean;
    function GetIsGlobal(): boolean;
    function GetIsMulticast(): boolean;
    function GetIsReserved(): boolean;
    function GetIsLoopback(): boolean; virtual;
    function GetIsLinkLocal(): boolean;
    function GetIsUnspecified(): boolean; virtual;
    function GetIsSiteLocal(): boolean;
    function GetIPv4Mapped(): IPAddressValue;
    function GetIPv6Mapped(): IPAddressValue;
    function GetSixToFour(): IPAddressValue;
    function GetTeredo(): IPPair;
  public
    constructor Create(value: object; requiredVersion: integer := 0);
    constructor Create(number: BigInteger; version: integer; scope: string := '');
    property value: BigInteger read number;
    property version: integer read GetVersion;
    property max_prefixlen: integer read GetMaxPrefixlen;
    property scope_id: string read zone;
    property packed: bytes read GetPacked;
    property compressed: string read GetCompressed;
    property exploded: string read GetExploded;
    property reverse_pointer: string read GetReversePointer;
    property is_private: boolean read GetIsPrivate;
    property is_global: boolean read GetIsGlobal;
    property is_multicast: boolean read GetIsMulticast;
    property is_reserved: boolean read GetIsReserved;
    property is_loopback: boolean read GetIsLoopback;
    property is_link_local: boolean read GetIsLinkLocal;
    property is_unspecified: boolean read GetIsUnspecified;
    property is_site_local: boolean read GetIsSiteLocal;
    property ipv4_mapped: IPAddressValue read GetIPv4Mapped;
    property ipv6_mapped: IPAddressValue read GetIPv6Mapped;
    property sixtofour: IPAddressValue read GetSixToFour;
    property teredo: IPPair read GetTeredo;
    function ToString(): string; override;
    function ToPythonInteger(): BigInteger;
    function Equals(other: object): boolean; override;
    function GetHashCode(): integer; override;
    function CompareTo(other: IPAddressValue): integer;
    static function operator =(left, right: IPAddressValue): boolean;
    static function operator <>(left, right: IPAddressValue): boolean;
    static function operator <(left, right: IPAddressValue): boolean;
    static function operator >(left, right: IPAddressValue): boolean;
    static function operator <=(left, right: IPAddressValue): boolean;
    static function operator >=(left, right: IPAddressValue): boolean;
    static function operator +(left: IPAddressValue; right: integer): IPAddressValue;
    static function operator -(left: IPAddressValue; right: integer): IPAddressValue;
  end;

  IPNetworkValue = class(IReadOnlyCollection<IPAddressValue>, IComparable<IPNetworkValue>)
  private
    start: BigInteger;
    family: integer;
    prefix: integer;
    zone: string;
    function GetVersion(): integer;
    function GetMaxPrefixlen(): integer;
    function GetNetworkAddress(): IPAddressValue;
    function GetBroadcastAddress(): IPAddressValue;
    function GetNetmask(): IPAddressValue;
    function GetHostmask(): IPAddressValue;
    function GetNumAddresses(): BigInteger;
    function GetCount(): integer;
    function GetItem(index: integer): IPAddressValue;
    function GetWithNetmask(): string;
    function GetWithHostmask(): string;
    function GetWithPrefixlen(): string;
    function GetCompressed(): string;
    function GetExploded(): string;
    function GetIsPrivate(): boolean;
    function GetIsGlobal(): boolean;
    function GetIsMulticast(): boolean;
    function GetIsReserved(): boolean;
    function GetIsLoopback(): boolean;
    function GetIsLinkLocal(): boolean;
    function GetIsUnspecified(): boolean;
    function GetIsSiteLocal(): boolean;
    function Items(): sequence of IPAddressValue;
  public
    constructor Create(value: object; requiredVersion: integer := 0; strict: boolean := true);
    constructor Create(value: BigInteger; version, prefixlen: integer);
    property version: integer read GetVersion;
    property max_prefixlen: integer read GetMaxPrefixlen;
    property prefixlen: integer read prefix;
    property network_address: IPAddressValue read GetNetworkAddress;
    property broadcast_address: IPAddressValue read GetBroadcastAddress;
    property netmask: IPAddressValue read GetNetmask;
    property hostmask: IPAddressValue read GetHostmask;
    property num_addresses: BigInteger read GetNumAddresses;
    property Count: integer read GetCount;
    property ByIndex[index: integer]: IPAddressValue read GetItem; default;
    property with_prefixlen: string read GetWithPrefixlen;
    property with_netmask: string read GetWithNetmask;
    property with_hostmask: string read GetWithHostmask;
    property compressed: string read GetCompressed;
    property exploded: string read GetExploded;
    property is_private: boolean read GetIsPrivate;
    property is_global: boolean read GetIsGlobal;
    property is_multicast: boolean read GetIsMulticast;
    property is_reserved: boolean read GetIsReserved;
    property is_loopback: boolean read GetIsLoopback;
    property is_link_local: boolean read GetIsLinkLocal;
    property is_unspecified: boolean read GetIsUnspecified;
    property is_site_local: boolean read GetIsSiteLocal;
    function ToString(): string; override;
    function GetEnumerator(): IEnumerator<IPAddressValue>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function Contains(address: IPAddressValue): boolean;
    function hosts(): sequence of IPAddressValue;
    function subnet_of(other: IPNetworkValue): boolean;
    function supernet_of(other: IPNetworkValue): boolean;
    function overlaps(other: IPNetworkValue): boolean;
    function subnets(prefixlen_diff: integer := 1; new_prefix: integer := -1): sequence of IPNetworkValue;
    function supernet(prefixlen_diff: integer := 1; new_prefix: integer := -1): IPNetworkValue;
    function address_exclude(other: IPNetworkValue): sequence of IPNetworkValue;
    function compare_networks(other: IPNetworkValue): integer;
    function Equals(other: object): boolean; override;
    function GetHashCode(): integer; override;
    function CompareTo(other: IPNetworkValue): integer;
    static function operator =(left, right: IPNetworkValue): boolean;
    static function operator <>(left, right: IPNetworkValue): boolean;
    static function operator <(left, right: IPNetworkValue): boolean;
    static function operator >(left, right: IPNetworkValue): boolean;
    static function operator <=(left, right: IPNetworkValue): boolean;
    static function operator >=(left, right: IPNetworkValue): boolean;
    static function operator in(address: IPAddressValue; network: IPNetworkValue): boolean;
    static function operator in(item: IPNetworkValue; network: IPNetworkValue): boolean;
  end;

  IPInterfaceValue = class(IPAddressValue)
  private
    net: IPNetworkValue;
    function GetIP(): IPAddressValue;
    function GetNetmask(): IPAddressValue;
    function GetHostmask(): IPAddressValue;
    function GetPrefixlen(): integer;
    function GetWithNetmask(): string;
    function GetWithHostmask(): string;
    function GetWithPrefixlen(): string;
    function GetExploded(): string; override;
    function GetIsLoopback(): boolean; override;
    function GetIsUnspecified(): boolean; override;
  public
    constructor Create(value: object; requiredVersion: integer := 0);
    property network: IPNetworkValue read net;
    property ip: IPAddressValue read GetIP;
    property netmask: IPAddressValue read GetNetmask;
    property hostmask: IPAddressValue read GetHostmask;
    property prefixlen: integer read GetPrefixlen;
    property with_prefixlen: string read GetWithPrefixlen;
    property with_netmask: string read GetWithNetmask;
    property with_hostmask: string read GetWithHostmask;
    function ToString(): string; override;
  end;

function ip_address(address: object): IPAddressValue;
function IPv4Address(address: object): IPAddressValue;
function IPv6Address(address: object): IPAddressValue;
function ip_network(address: object; strict: boolean := true): IPNetworkValue;
function IPv4Network(address: object; strict: boolean := true): IPNetworkValue;
function IPv6Network(address: object; strict: boolean := true): IPNetworkValue;
function ip_interface(address: object): IPInterfaceValue;
function IPv4Interface(address: object): IPInterfaceValue;
function IPv6Interface(address: object): IPInterfaceValue;
function v4_int_to_packed(value: BigInteger): bytes;
function v6_int_to_packed(value: BigInteger): bytes;
function summarize_address_range(first, last: IPAddressValue): sequence of IPNetworkValue;
function collapse_addresses(addresses: System.Collections.IEnumerable): sequence of IPNetworkValue;
function get_mixed_type_key(value: object): IPMixedKey;

implementation

constructor IPPair.Create(first, second: IPAddressValue);
begin
  self.first := first;
  self.second := second;
end;
function IPPair.GetItem(index: integer): IPAddressValue;
begin
  if index < 0 then index += 2;
  if index = 0 then exit(first);
  if index = 1 then exit(second);
  raise new System.IndexOutOfRangeException('tuple index out of range');
end;
function IPPair.GetCount(): integer := 2;
function IPPair.GetEnumerator(): IEnumerator<IPAddressValue>;
begin
  var values := new IPAddressValue[2];
  values[0] := first;
  values[1] := second;
  Result := (values as IEnumerable<IPAddressValue>).GetEnumerator();
end;
function IPPair.ToString(): string :=
  '(IPv4Address(''' + first.ToString() + '''), IPv4Address(''' + second.ToString() + '''))';

constructor IPMixedKey.Create(value: IPAddressValue);
begin
  family := value.version;
  number := value.value;
  mask := -1;
  network := false;
  var className := if family = 4 then 'IPv4Address' else 'IPv6Address';
  representation := '(' + family.ToString() + ', ' + className + '(''' + value.ToString() + '''))';
end;

constructor IPMixedKey.Create(value: IPNetworkValue);
begin
  family := value.version;
  number := value.network_address.value;
  mask := value.netmask.value;
  network := true;
  var className := if family = 4 then 'IPv4Address' else 'IPv6Address';
  representation := '(' + family.ToString() + ', ' + className + '(''' +
    value.network_address.ToString() + '''), ' + className + '(''' +
    value.netmask.ToString() + '''))';
end;

function IPMixedKey.CompareTo(other: IPMixedKey): integer;
begin
  Result := family.CompareTo(other.family);
  if Result <> 0 then exit;
  Result := number.CompareTo(other.number);
  if Result <> 0 then exit;
  if network <> other.network then exit(if network then 1 else -1);
  Result := mask.CompareTo(other.mask);
end;
function IPMixedKey.ToString(): string := representation;

function ParseAddress(value: object; requiredVersion: integer; var version: integer; var scope: string): BigInteger;
begin
  if value is PyValue then value := (value as PyValue).value;
  scope := '';
  if value is IPAddressValue then
  begin
    var other := value as IPAddressValue;
    version := other.version;
    scope := other.scope_id;
    Result := other.value;
  end
  else if value is bytes then
  begin
    var data := (value as bytes).to_array();
    version := if data.Length = 4 then 4 else if data.Length = 16 then 6 else 0;
    if version = 0 then raise new AddressValueError('Invalid packed IP address length');
    Result := new BigInteger(0);
    foreach var b in data do Result := (Result shl 8) + b;
  end
  else if (value is integer) or (value is int64) or (value is uint64) or (value is BigInteger) then
  begin
    version := if requiredVersion = 0 then 4 else requiredVersion;
    Result := BigInteger.Parse(value.ToString());
  end
  else
  begin
    var s := value.ToString();
    if s.Contains('/') then raise new AddressValueError('Unexpected ''/'' in address');
    version := if s.Contains(':') then 6 else 4;
    if version = 4 then
    begin
      var parts := s.Split('.');
      if parts.Length <> 4 then raise new AddressValueError('Invalid IPv4 address');
      Result := new BigInteger(0);
      foreach var part in parts do
      begin
        if (part.Length = 0) or (part.Length > 3) or ((part.Length > 1) and (part[1] = '0')) then
          raise new AddressValueError('Invalid IPv4 address');
        foreach var c in part do
          if (c < '0') or (c > '9') then raise new AddressValueError('Invalid IPv4 address');
        var octet: integer;
        if not integer.TryParse(part, octet) or (octet > 255) then
          raise new AddressValueError('Invalid IPv4 address');
        Result := (Result shl 8) + octet;
      end;
    end
    else
    begin
      var percent := s.IndexOf('%');
      if percent >= 0 then
      begin
        scope := s.Substring(percent + 1);
        if (scope.Length = 0) or scope.Contains('%') then
          raise new AddressValueError('Invalid IPv6 scope ID');
        s := s.Substring(0, percent);
      end;
      foreach var c in s do
        if not ('0123456789abcdefABCDEF:.'.Contains(c.ToString())) then
          raise new AddressValueError('Invalid IPv6 address');
      var parsed: System.Net.IPAddress;
      if not System.Net.IPAddress.TryParse(s, parsed) or (parsed.AddressFamily <> System.Net.Sockets.AddressFamily.InterNetworkV6) then
        raise new AddressValueError('Invalid IPv6 address');
      Result := new BigInteger(0);
      foreach var b in parsed.GetAddressBytes() do Result := (Result shl 8) + b;
    end;
  end;
  if (requiredVersion <> 0) and (version <> requiredVersion) then
    raise new AddressValueError('Address has the wrong IP version');
  if (Result < 0) or (Result >= BigInteger.Pow(new BigInteger(2), if version = 4 then 32 else 128)) then
    raise new AddressValueError('IP address is out of range');
end;

constructor IPAddressValue.Create(value: object; requiredVersion: integer);
begin
  number := ParseAddress(value, requiredVersion, family, zone);
end;

constructor IPAddressValue.Create(number: BigInteger; version: integer; scope: string);
begin
  if (version <> 4) and (version <> 6) then raise new AddressValueError('Invalid IP version');
  if (number < 0) or (number >= BigInteger.Pow(new BigInteger(2), if version = 4 then 32 else 128)) then
    raise new AddressValueError('IP address is out of range');
  self.number := number;
  family := version;
  zone := scope;
end;

function IPAddressValue.GetVersion(): integer := family;
function IPAddressValue.ToPythonInteger(): BigInteger := number;
function IPAddressValue.GetMaxPrefixlen(): integer := if family = 4 then 32 else 128;

function IPAddressValue.GetPacked(): bytes;
begin
  var data := new byte[max_prefixlen div 8];
  var n := number;
  for var i := data.Length - 1 downto 0 do
  begin
    data[i] := byte(n mod 256);
    n := n shr 8;
  end;
  Result := new bytes(data);
end;

function IPAddressValue.ToString(): string;
begin
  var raw := packed.to_array();
  if family = 4 then
    Result := System.String.Join('.', raw.Select(b -> b.ToString()).ToArray())
  else
  begin
    var parsed := new System.Net.IPAddress(raw);
    Result := parsed.ToString();
  end;
  if zone <> '' then Result += '%' + zone;
end;

function IPAddressValue.GetCompressed(): string := ToString();

function IPAddressValue.Equals(other: object): boolean;
begin
  if not (other is IPAddressValue) then exit(false);
  var address := other as IPAddressValue;
  Result := (family = address.version) and (number = address.value) and
    (zone = address.scope_id) and ((self is IPInterfaceValue) = (other is IPInterfaceValue));
  if Result and (self is IPInterfaceValue) then
    Result := (self as IPInterfaceValue).network.Equals((other as IPInterfaceValue).network);
end;

function IPAddressValue.GetHashCode(): integer :=
  number.GetHashCode() xor family.GetHashCode() xor zone.GetHashCode();

function IPAddressValue.CompareTo(other: IPAddressValue): integer;
begin
  if family <> other.version then raise new TypeError('IP versions differ');
  Result := number.CompareTo(other.value);
end;

static function IPAddressValue.operator =(left, right: IPAddressValue): boolean;
begin
  if System.Object.ReferenceEquals(left, right) then exit(true);
  if System.Object.ReferenceEquals(left, nil) or System.Object.ReferenceEquals(right, nil) then exit(false);
  Result := left.Equals(right);
end;
static function IPAddressValue.operator <>(left, right: IPAddressValue): boolean := not (left = right);
static function IPAddressValue.operator <(left, right: IPAddressValue): boolean :=
  left.CompareTo(right) < 0;
static function IPAddressValue.operator >(left, right: IPAddressValue): boolean := right < left;
static function IPAddressValue.operator <=(left, right: IPAddressValue): boolean := (left < right) or (left = right);
static function IPAddressValue.operator >=(left, right: IPAddressValue): boolean := (left > right) or (left = right);
static function IPAddressValue.operator +(left: IPAddressValue; right: integer): IPAddressValue :=
  new IPAddressValue(left.value + right, left.version);
static function IPAddressValue.operator -(left: IPAddressValue; right: integer): IPAddressValue :=
  new IPAddressValue(left.value - right, left.version);


function IPAddressValue.GetExploded(): string;
begin
  if family = 4 then exit(ToString());
  var data := packed.to_array();
  var parts := new List<string>;
  for var i := 0 to 7 do
    parts.Add(((integer(data[2 * i]) shl 8) + data[2 * i + 1]).ToString('x4'));
  if ipv4_mapped <> nil then
    Result := System.String.Join(':', parts.Take(6).ToArray()) + ':' + ipv4_mapped.ToString()
  else Result := System.String.Join(':', parts.ToArray());
  if zone <> '' then Result += '%' + zone;
end;

function IPAddressValue.GetReversePointer(): string;
begin
  if family = 4 then
  begin
    var raw := packed.to_array();
    exit($'{raw[3]}.{raw[2]}.{raw[1]}.{raw[0]}.in-addr.arpa');
  end;
  var rawHex := number.ToString('x').TrimStart('0').PadLeft(32, '0');
  var parts := new List<string>;
  for var i := rawHex.Length downto 1 do parts.Add(rawHex[i].ToString());
  Result := System.String.Join('.', parts.ToArray()) + '.ip6.arpa';
end;

function InCIDR(value: BigInteger; version: integer; spec: string): boolean;
begin
  var n := new IPNetworkValue(spec, version, true);
  Result := (value >= n.network_address.value) and (value <= n.broadcast_address.value);
end;

function InAnyCIDR(value: BigInteger; version: integer; specs: array of string): boolean;
begin
  foreach var spec in specs do if InCIDR(value, version, spec) then exit(true);
  Result := false;
end;

function FullyInAnyCIDR(first, last: BigInteger; version: integer; specs: array of string): boolean;
begin
  foreach var spec in specs do
    if InCIDR(first, version, spec) and InCIDR(last, version, spec) then exit(true);
  Result := false;
end;

function IPAddressValue.GetIPv4Mapped(): IPAddressValue;
begin
  Result := nil;
  if (family = 6) and (number shr 32 = 65535) then
    Result := new IPAddressValue(number mod BigInteger.Pow(new BigInteger(2), 32), 4);
end;

function IPAddressValue.GetIPv6Mapped(): IPAddressValue;
begin
  Result := nil;
  if family = 4 then Result := new IPAddressValue((new BigInteger(65535) shl 32) + number, 6);
end;

function IPAddressValue.GetSixToFour(): IPAddressValue;
begin
  Result := nil;
  if (family = 6) and (number shr 112 = 8194) then
    Result := new IPAddressValue((number shr 80) mod BigInteger.Pow(new BigInteger(2), 32), 4);
end;

function IPAddressValue.GetTeredo(): IPPair;
begin
  Result := nil;
  if (family <> 6) or (number shr 96 <> 536936448) then exit;
  var modulus := BigInteger.Pow(new BigInteger(2), 32);
  var server := new IPAddressValue((number shr 64) mod modulus, 4);
  var client := new IPAddressValue((modulus - 1) - number mod modulus, 4);
  Result := new IPPair(server, client);
end;

function IPAddressValue.GetIsPrivate(): boolean;
begin
  if ipv4_mapped <> nil then exit(ipv4_mapped.is_private);
  if family = 4 then
    Result := InAnyCIDR(number, 4, |'0.0.0.0/8','10.0.0.0/8','127.0.0.0/8',
      '169.254.0.0/16','172.16.0.0/12','192.0.0.0/24','192.0.2.0/24',
      '192.168.0.0/16','198.18.0.0/15','198.51.100.0/24',
      '203.0.113.0/24','240.0.0.0/4'|)
      and not InAnyCIDR(number, 4, |'192.0.0.9/32','192.0.0.10/32'|)
  else
    Result := InAnyCIDR(number, 6, |'::1/128','::/128','::ffff:0:0/96',
      '64:ff9b:1::/48','100::/64','2001::/23','2001:db8::/32',
      '2002::/16','3fff::/20','fc00::/7','fe80::/10'|)
      and not InAnyCIDR(number, 6, |'2001:1::1/128','2001:1::2/128',
        '2001:3::/32','2001:4:112::/48','2001:20::/28','2001:30::/28'|);
end;

function IPAddressValue.GetIsGlobal(): boolean;
begin
  if ipv4_mapped <> nil then exit(ipv4_mapped.is_global);
  Result := not is_private;
  if (family = 4) and InCIDR(number, 4, '100.64.0.0/10') then Result := false;
end;

function IPAddressValue.GetIsMulticast(): boolean :=
  if ipv4_mapped <> nil then ipv4_mapped.is_multicast
  else if family = 4 then InCIDR(number, 4, '224.0.0.0/4')
  else InCIDR(number, 6, 'ff00::/8');

function IPAddressValue.GetIsReserved(): boolean :=
  if ipv4_mapped <> nil then ipv4_mapped.is_reserved
  else if family = 4 then InCIDR(number, 4, '240.0.0.0/4')
  else InAnyCIDR(number, 6, |'::/8','100::/8','200::/7','400::/6',
    '800::/5','1000::/4','4000::/3','6000::/3','8000::/3',
    'a000::/3','c000::/3','e000::/4','f000::/5','f800::/6','fe00::/9'|);

function IPAddressValue.GetIsLoopback(): boolean :=
  if ipv4_mapped <> nil then ipv4_mapped.is_loopback
  else if family = 4 then InCIDR(number, 4, '127.0.0.0/8')
  else number = 1;

function IPAddressValue.GetIsLinkLocal(): boolean :=
  if ipv4_mapped <> nil then ipv4_mapped.is_link_local
  else if family = 4 then InCIDR(number, 4, '169.254.0.0/16')
  else InCIDR(number, 6, 'fe80::/10');

function IPAddressValue.GetIsUnspecified(): boolean :=
  if ipv4_mapped <> nil then ipv4_mapped.is_unspecified else number = 0;

function IPAddressValue.GetIsSiteLocal(): boolean :=
  (family = 6) and InCIDR(number, 6, 'fec0::/10');

function ParsePrefix(mask: string; version: integer): integer;
begin
  var bits := if version = 4 then 32 else 128;
  if mask = '' then exit(bits);
  if (version = 4) and mask.Contains('.') then
  begin
    var parsedMask := new IPAddressValue(mask, 4);
    var n := parsedMask.value;
    var maximum := BigInteger.Pow(new BigInteger(2), 32) - 1;
    if (n <> 0) and ((n shr 24) = 0) then n := maximum - n;
    Result := 0;
    var seenZero := false;
    for var i := 31 downto 0 do
    begin
      if ((n shr i) mod 2) = 0 then seenZero := true
      else if seenZero then raise new NetmaskValueError('Invalid netmask')
      else Result += 1;
    end;
    exit;
  end;
  foreach var c in mask do
    if (c < '0') or (c > '9') then raise new NetmaskValueError('Invalid prefix length');
  if not integer.TryParse(mask, Result) or (Result > bits) or (Result < 0) then
    raise new NetmaskValueError('Invalid prefix length');
end;

function IsAddressPrefixTuple(value: object): boolean :=
  (value <> nil) and (value.GetType().FullName <> nil) and
  value.GetType().FullName.StartsWith('System.Tuple`2');

function TupleItem(value: object; itemNumber: integer): object :=
  value.GetType().GetProperty('Item' + itemNumber.ToString()).GetValue(value, nil);

constructor IPNetworkValue.Create(value: object; requiredVersion: integer; strict: boolean);
begin
  if value is PyValue then value := (value as PyValue).value;
  var s := value.ToString();
  var slash := s.IndexOf('/');
  if (slash >= 0) and (s.IndexOf('/', slash + 1) >= 0) then
    raise new AddressValueError('Invalid network address');
  var addressText := if slash < 0 then s else s.Substring(0, slash);
  var maskText := if slash < 0 then '' else s.Substring(slash + 1);
  var addressValue: object := if slash < 0 then value else addressText;
  if IsAddressPrefixTuple(value) then
  begin
    addressValue := TupleItem(value, 1);
    addressText := addressValue.ToString();
    maskText := TupleItem(value, 2).ToString();
  end;
  var address := new IPAddressValue(addressValue, requiredVersion);
  family := address.version;
  zone := address.scope_id;
  prefix := ParsePrefix(maskText, family);
  var hostbits := BigInteger.Pow(new BigInteger(2), max_prefixlen - prefix) - 1;
  start := address.value div (hostbits + 1) * (hostbits + 1);
  if strict and (start <> address.value) then
    raise new ValueError(addressText + '/' + prefix.ToString() + ' has host bits set');
end;

constructor IPNetworkValue.Create(value: BigInteger; version, prefixlen: integer);
begin
  family := version;
  prefix := prefixlen;
  start := value;
  zone := '';
end;

function IPNetworkValue.GetVersion(): integer := family;
function IPNetworkValue.GetMaxPrefixlen(): integer := if family = 4 then 32 else 128;
function IPNetworkValue.GetNetworkAddress(): IPAddressValue := new IPAddressValue(start, family, zone);
function IPNetworkValue.GetNumAddresses(): BigInteger := BigInteger.Pow(new BigInteger(2), max_prefixlen - prefix);
function IPNetworkValue.GetBroadcastAddress(): IPAddressValue := new IPAddressValue(start + num_addresses - 1, family);
function IPNetworkValue.GetNetmask(): IPAddressValue :=
  new IPAddressValue(BigInteger.Pow(new BigInteger(2), max_prefixlen) - num_addresses, family);
function IPNetworkValue.GetHostmask(): IPAddressValue := new IPAddressValue(num_addresses - 1, family);

function IPNetworkValue.ToString(): string := network_address.ToString() + '/' + prefix.ToString();
function IPNetworkValue.GetWithPrefixlen(): string := ToString();
function IPNetworkValue.GetCompressed(): string := ToString();
function IPNetworkValue.GetExploded(): string := network_address.exploded + '/' + prefix.ToString();
function IPNetworkValue.GetWithNetmask(): string := network_address.ToString() + '/' + netmask.ToString();
function IPNetworkValue.GetWithHostmask(): string := network_address.ToString() + '/' + hostmask.ToString();
function IPNetworkValue.GetCount(): integer;
begin
  if num_addresses > integer.MaxValue then raise new System.OverflowException('network too large');
  Result := integer(num_addresses);
end;

function IPNetworkValue.GetItem(index: integer): IPAddressValue;
begin
  var offset := new BigInteger(index);
  if offset < 0 then offset += num_addresses;
  if (offset < 0) or (offset >= num_addresses) then
    raise new System.IndexOutOfRangeException('address out of range');
  Result := new IPAddressValue(start + offset, family);
end;

function IPNetworkValue.Items(): sequence of IPAddressValue;
begin
  var current := start;
  var finish := start + num_addresses;
  while current < finish do
  begin
    yield new IPAddressValue(current, family);
    current += 1;
  end;
end;

function IPNetworkValue.GetEnumerator(): IEnumerator<IPAddressValue> := Items().GetEnumerator();

function IPNetworkValue.Contains(address: IPAddressValue): boolean :=
  (address <> nil) and (address.version = family) and
  (address.value >= start) and (address.value < start + num_addresses);

function IPNetworkValue.hosts(): sequence of IPAddressValue;
begin
  var first := start + 1;
  var finish := start + num_addresses - 1;
  if family = 4 then
  begin
    if prefix >= 31 then begin first := start; finish := start + num_addresses; end;
  end
  else begin finish := start + num_addresses; if prefix >= 127 then first := start; end;
  while first < finish do
  begin
    yield new IPAddressValue(first, family);
    first += 1;
  end;
end;

function IPNetworkValue.subnet_of(other: IPNetworkValue): boolean;
begin
  if family <> other.version then raise new TypeError('IP versions differ');
  Result := (start >= other.start) and
    (start + num_addresses <= other.start + other.num_addresses);
end;

function IPNetworkValue.supernet_of(other: IPNetworkValue): boolean := other.subnet_of(self);

function IPNetworkValue.overlaps(other: IPNetworkValue): boolean :=
  (family = other.version) and (start < other.start + other.num_addresses) and
  (other.start < start + num_addresses);

function IPNetworkValue.subnets(prefixlen_diff: integer; new_prefix: integer): sequence of IPNetworkValue;
begin
  if prefix = max_prefixlen then begin yield self; exit; end;
  if new_prefix >= 0 then
  begin
    if new_prefix < prefix then raise new ValueError('new prefix must be longer');
    if prefixlen_diff <> 1 then raise new ValueError('cannot set both prefix options');
    prefixlen_diff := new_prefix - prefix;
  end;
  if (prefixlen_diff < 0) or (prefix + prefixlen_diff > max_prefixlen) then
    raise new ValueError('Invalid prefix length difference');
  var size := BigInteger.Pow(new BigInteger(2), max_prefixlen - prefix - prefixlen_diff);
  var current := start;
  var finish := start + num_addresses;
  while current < finish do
  begin
    yield new IPNetworkValue(current, family, prefix + prefixlen_diff);
    current += size;
  end;
end;

function IPNetworkValue.supernet(prefixlen_diff: integer; new_prefix: integer): IPNetworkValue;
begin
  if prefix = 0 then exit(self);
  if new_prefix >= 0 then
  begin
    if new_prefix > prefix then raise new ValueError('new prefix must be shorter');
    if prefixlen_diff <> 1 then raise new ValueError('cannot set both prefix options');
    prefixlen_diff := prefix - new_prefix;
  end;
  if (prefixlen_diff < 0) or (prefixlen_diff > prefix) then
    raise new ValueError('Invalid prefix length difference');
  var size := BigInteger.Pow(new BigInteger(2), max_prefixlen - prefix + prefixlen_diff);
  Result := new IPNetworkValue(start div size * size, family, prefix - prefixlen_diff);
end;

function IPNetworkValue.address_exclude(other: IPNetworkValue): sequence of IPNetworkValue;
begin
  if not other.subnet_of(self) then raise new ValueError('network is not contained');
  if (start = other.start) and (prefix = other.prefix) then exit;
  var current := self;
  while current.prefix < other.prefix do
  begin
    var children := current.subnets().ToArray();
    if other.subnet_of(children[0]) then
    begin
      yield children[1];
      current := children[0];
    end
    else
    begin
      yield children[0];
      current := children[1];
    end;
  end;
end;

function IPNetworkValue.compare_networks(other: IPNetworkValue): integer;
begin
  if family <> other.version then raise new TypeError('IP versions differ');
  if start < other.start then exit(-1);
  if start > other.start then exit(1);
  if prefix < other.prefix then exit(-1);
  if prefix > other.prefix then exit(1);
  Result := 0;
end;

function IPNetworkValue.Equals(other: object): boolean;
begin
  if not (other is IPNetworkValue) then exit(false);
  var network := other as IPNetworkValue;
  Result := (family = network.version) and (start = network.start) and
    (prefix = network.prefixlen) and (zone = network.zone);
end;

function IPNetworkValue.GetHashCode(): integer :=
  start.GetHashCode() xor family.GetHashCode() xor prefix.GetHashCode() xor zone.GetHashCode();
function IPNetworkValue.CompareTo(other: IPNetworkValue): integer := compare_networks(other);
static function IPNetworkValue.operator =(left, right: IPNetworkValue): boolean;
begin
  if System.Object.ReferenceEquals(left, right) then exit(true);
  if System.Object.ReferenceEquals(left, nil) or System.Object.ReferenceEquals(right, nil) then exit(false);
  Result := left.Equals(right);
end;
static function IPNetworkValue.operator <>(left, right: IPNetworkValue): boolean := not (left = right);
static function IPNetworkValue.operator <(left, right: IPNetworkValue): boolean :=
  left.compare_networks(right) < 0;
static function IPNetworkValue.operator >(left, right: IPNetworkValue): boolean :=
  left.compare_networks(right) > 0;
static function IPNetworkValue.operator <=(left, right: IPNetworkValue): boolean :=
  left.compare_networks(right) <= 0;
static function IPNetworkValue.operator >=(left, right: IPNetworkValue): boolean :=
  left.compare_networks(right) >= 0;
static function IPNetworkValue.operator in(address: IPAddressValue; network: IPNetworkValue): boolean :=
  network.Contains(address);
static function IPNetworkValue.operator in(item: IPNetworkValue; network: IPNetworkValue): boolean := false;

function IPNetworkValue.GetIsPrivate(): boolean;
begin
  var last := start + num_addresses - 1;
  if family = 4 then
  begin
    Result := FullyInAnyCIDR(start, last, 4, |'0.0.0.0/8','10.0.0.0/8','127.0.0.0/8',
      '169.254.0.0/16','172.16.0.0/12','192.0.0.0/24','192.0.2.0/24',
      '192.168.0.0/16','198.18.0.0/15','198.51.100.0/24',
      '203.0.113.0/24','240.0.0.0/4'|)
      and not InAnyCIDR(start, 4, |'192.0.0.9/32','192.0.0.10/32'|)
      and not InAnyCIDR(last, 4, |'192.0.0.9/32','192.0.0.10/32'|);
  end
  else
    Result := FullyInAnyCIDR(start, last, 6, |'::1/128','::/128','::ffff:0:0/96',
      '64:ff9b:1::/48','100::/64','2001::/23','2001:db8::/32',
      '2002::/16','3fff::/20','fc00::/7','fe80::/10'|)
      and not InAnyCIDR(start, 6, |'2001:1::1/128','2001:1::2/128',
        '2001:3::/32','2001:4:112::/48','2001:20::/28','2001:30::/28'|)
      and not InAnyCIDR(last, 6, |'2001:1::1/128','2001:1::2/128',
        '2001:3::/32','2001:4:112::/48','2001:20::/28','2001:30::/28'|);
end;

function IPNetworkValue.GetIsGlobal(): boolean :=
  not is_private and not ((family = 4) and
    InCIDR(start, 4, '100.64.0.0/10') and InCIDR(broadcast_address.value, 4, '100.64.0.0/10'));
function IPNetworkValue.GetIsMulticast(): boolean := network_address.is_multicast and broadcast_address.is_multicast;
function IPNetworkValue.GetIsReserved(): boolean := network_address.is_reserved and broadcast_address.is_reserved;
function IPNetworkValue.GetIsLoopback(): boolean := network_address.is_loopback and broadcast_address.is_loopback;
function IPNetworkValue.GetIsLinkLocal(): boolean := network_address.is_link_local and broadcast_address.is_link_local;
function IPNetworkValue.GetIsUnspecified(): boolean := network_address.is_unspecified and broadcast_address.is_unspecified;
function IPNetworkValue.GetIsSiteLocal(): boolean := network_address.is_site_local and broadcast_address.is_site_local;

function AddressPart(value: object): object;
begin
  if IsAddressPrefixTuple(value) then exit(TupleItem(value, 1));
  var s := value.ToString();
  var slash := s.IndexOf('/');
  Result := if slash < 0 then s else s.Substring(0, slash);
end;

constructor IPInterfaceValue.Create(value: object; requiredVersion: integer);
begin
  inherited Create(AddressPart(value), requiredVersion);
  var source := value.ToString();
  if IsAddressPrefixTuple(value) then
    source := AddressPart(value).ToString() + '/' + TupleItem(value, 2).ToString();
  if scope_id <> '' then source := source.Replace('%' + scope_id, '');
  net := new IPNetworkValue(source, requiredVersion, false);
end;

function IPInterfaceValue.GetIP(): IPAddressValue := new IPAddressValue(value, version);
function IPInterfaceValue.GetNetmask(): IPAddressValue := net.netmask;
function IPInterfaceValue.GetHostmask(): IPAddressValue := net.hostmask;
function IPInterfaceValue.GetPrefixlen(): integer := net.prefixlen;
function IPInterfaceValue.ToString(): string := inherited ToString() + '/' + prefixlen.ToString();
function IPInterfaceValue.GetWithPrefixlen(): string := ip.ToString() + '/' + prefixlen.ToString();
function IPInterfaceValue.GetExploded(): string := ip.exploded + '/' + prefixlen.ToString();
function IPInterfaceValue.GetIsLoopback(): boolean :=
  inherited GetIsLoopback() and ((version = 4) or net.is_loopback);
function IPInterfaceValue.GetIsUnspecified(): boolean :=
  inherited GetIsUnspecified() and ((version = 4) or net.is_unspecified);
function IPInterfaceValue.GetWithNetmask(): string := ip.ToString() + '/' + netmask.ToString();
function IPInterfaceValue.GetWithHostmask(): string := ip.ToString() + '/' + hostmask.ToString();

function ip_address(address: object): IPAddressValue;
begin
  try Result := new IPAddressValue(address, 4)
  except on AddressValueError do Result := new IPAddressValue(address, 6);
  end;
end;

function IPv4Address(address: object): IPAddressValue := new IPAddressValue(address, 4);
function IPv6Address(address: object): IPAddressValue := new IPAddressValue(address, 6);

function ip_network(address: object; strict: boolean): IPNetworkValue;
begin
  try Result := new IPNetworkValue(address, 4, strict)
  except on AddressValueError do Result := new IPNetworkValue(address, 6, strict);
  end;
end;

function IPv4Network(address: object; strict: boolean): IPNetworkValue :=
  new IPNetworkValue(address, 4, strict);
function IPv6Network(address: object; strict: boolean): IPNetworkValue :=
  new IPNetworkValue(address, 6, strict);

function ip_interface(address: object): IPInterfaceValue;
begin
  try Result := new IPInterfaceValue(address, 4)
  except on AddressValueError do Result := new IPInterfaceValue(address, 6);
  end;
end;

function IPv4Interface(address: object): IPInterfaceValue := new IPInterfaceValue(address, 4);
function IPv6Interface(address: object): IPInterfaceValue := new IPInterfaceValue(address, 6);

function v4_int_to_packed(value: BigInteger): bytes;
begin
  var address := new IPAddressValue(value, 4);
  Result := address.packed;
end;
function v6_int_to_packed(value: BigInteger): bytes;
begin
  var address := new IPAddressValue(value, 6);
  Result := address.packed;
end;

function summarize_address_range(first, last: IPAddressValue): sequence of IPNetworkValue;
begin
  if first.version <> last.version then raise new TypeError('IP versions differ');
  if first.value > last.value then raise new ValueError('last address must be greater');
  var current := first.value;
  var finish := last.value;
  var bits := first.max_prefixlen;
  while current <= finish do
  begin
    var zeroBits := 0;
    var probe := current;
    if probe = 0 then zeroBits := bits
    else while (zeroBits < bits) and (probe mod 2 = 0) do
    begin
      zeroBits += 1;
      probe := probe shr 1;
    end;
    var remaining := finish - current + 1;
    var blockSize := BigInteger.Pow(new BigInteger(2), zeroBits);
    while blockSize > remaining do
    begin
      zeroBits -= 1;
      blockSize := blockSize shr 1;
    end;
    yield new IPNetworkValue(current, first.version, bits - zeroBits);
    current += blockSize;
  end;
end;

function collapse_addresses(addresses: System.Collections.IEnumerable): sequence of IPNetworkValue;
begin
  var ranges := new List<IPNetworkValue>;
  var version := 0;
  var cursor := addresses.GetEnumerator();
  while cursor.MoveNext() do
  begin
    var item := cursor.Current;
    if item is PyValue then item := (item as PyValue).value;
    var n: IPNetworkValue;
    if item is IPNetworkValue then n := item as IPNetworkValue
    else if item is IPAddressValue then
    begin
      var a := item as IPAddressValue;
      n := new IPNetworkValue(a.value, a.version, a.max_prefixlen);
    end
    else raise new TypeError('Expected IP addresses or networks');
    if (version <> 0) and (n.version <> version) then
      raise new TypeError('IP versions differ');
    version := n.version;
    ranges.Add(n);
  end;
  var ordered := ranges.OrderBy(n -> n.network_address.value).ThenBy(n -> n.prefixlen).ToArray();
  if ordered.Length = 0 then exit;
  var first := ordered[0].network_address.value;
  var last := ordered[0].broadcast_address.value;
  for var i := 1 to ordered.Length - 1 do
  begin
    var nextFirst := ordered[i].network_address.value;
    var nextLast := ordered[i].broadcast_address.value;
    if nextFirst <= last + 1 then
    begin
      if nextLast > last then last := nextLast;
    end
    else
    begin
      foreach var n in summarize_address_range(new IPAddressValue(first, version),
          new IPAddressValue(last, version)) do yield n;
      first := nextFirst;
      last := nextLast;
    end;
  end;
  foreach var n in summarize_address_range(new IPAddressValue(first, version),
      new IPAddressValue(last, version)) do yield n;
end;

function get_mixed_type_key(value: object): IPMixedKey;
begin
  if value is PyValue then value := (value as PyValue).value;
  if value is IPNetworkValue then exit(new IPMixedKey(value as IPNetworkValue));
  if value is IPAddressValue then exit(new IPMixedKey(value as IPAddressValue));
  Result := nil;
end;

end.
