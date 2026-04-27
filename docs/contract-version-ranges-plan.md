# Goal

I want to update cppwinrt so that it takes a "-min_platform" and "-max_platform" commandline parameter.
Those point to the file paths like "platform.xml" in the Windows SDK containing a list of contract
identifiers. When cppwinrt projects types, any types in contracts that are <= the "min platform" are
emitted normally. Interfaces that are "min_platform < contract <= max_platform" are projected in the
usual way, but runtimeclass projections exclude those interface definitions. The class projection can
be cast via static_cast<> or .try_as<> to an interface in that "middle range".

## Example

```idl3
[contractversion(4)] apicontract FooContract {};

[contract(FooContract, 1)]
runtimeclass IAlwaysVisible {
    String Do();
}

[contract(FooContract, 3)]
runtimeclass ISometimesVisible {
    void Another();
}

[contract(FooContract, 4)]
runtimeclass INotProjected {
    void Oops();
}

[contract(FooContract, 1)]
runtimeclass MyTypeBase : [contract(FooContract, 4)] INotProjected
{
    MyTypeBase();

    void AlwaysVisible();

    [contract(FooContract, 2)]
    {
        String Smell { get; };
        Int64 Size;
    }

    [contract(FooContract, 3)]
    void Grump();

    [contract(FooContract, 4)]
    {
        IVector<IAlwaysVisible> GetThings();
        IMap<String, INotProjected> MoreThings;
    }
}
```

"Baseline" version contract:

```xml
<ContractMap>
    <Contract Name="FooContract" Version="1"/>
</ContractMap>
```

"Max" version contract:

```xml
<ContractMap>
    <Contract Name="FooContract" Version="3"/>
</ContractMap>
```

When passing that `-min_platform baseline.xml -max_platform max.xml`, the following are projected:

* IAlwaysVisible
* ISometimesVisible
* MyTypeBase
    * Constructor
    * AlwaysVisible

These are castable from MyTypeBase, but NOT directly projected:

* MyTypeBase
    * Smell, Size
    * Grump

These are NOT projected at all since they are outside the contract range:

* INotProjected
* MyTypeBase
    * INotProjected as a base type
    * GetThings
    * MoreThings

The developer can say this:

```c++
MyTypeBase base;
base.AlwaysVisible();
// The generated "-2" interface is castable
if (auto v = base.try_as<IMyTypeBase2>()) {
    v.Smell(L"kittens");
    v.Size(v.Size() += 2);
}
if (auto j = base.try_as<IMyTypeBase3>()) {
    v.Grump();
}
```

But these all fail:

```c++
MyTypeBase base;
base.Oops(); // INotProjected not available in MyTypeBase
(void)base.GetThings(); // MyTypeBase contract version 4 not projected
static_cast<IMyTypeBase4>(base).MoreThings(); // IMyTypeBase not projected
```