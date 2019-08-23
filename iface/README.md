Wire Cell Interfaces
====================

Overview
--------

This package provides a set of classes which define the interface to
Wire Cell.  Software from the other Wire Cell packages that provides
Wire Cell functionality do so through implementing these interfaces
and by calling into them.  Outside "client" code that wishes to use
Wire Cell code directly should do so by compiling and linking against
this package.  In general there should not be direct compilation or
linking against any higher level package in Wire Cell.

Component Interfaces
--------------------

Major facets of Wire Cell inherit from a subclass of
`WireCell::Interface` (which is actually provided by `util`).  Such
classes have a (non-const) `shared_ptr` defined by the base class as
these facets tend to be shared by multiple client code.  The concrete
implementation of an Interface register through the
`WireCell::NamedFactory` mechanism which allows dynamic construction.

Data Interfaces
---------------

The data objects which Wire Cell produces and operates on are defined
through interface classes as well.  Their interfaces do not inherent
from `WireCell::Interface` as components.  Rather, all data interface
classes inherit from `WireCell::IData<Data>`, which is templated on
the data interface class itself.  See for example `WireCell::IWire`.

Every data interface class has a number of types defined including a
(const) `shared_ptr`, a base iterator as well as an abstract iterator
and iterator range.  Also defined are types for Wire Cell signal/slot
mechanism which is described more below.

Some specific data interface also define some types for convenience
such as providing a common vector collection.  These types follow a
common naming convention, for example: `WireCell::IWireVector`,
`WireCell::ICellVector` and `WireCell::IPlaneSliceVector`.

There are also interfaces for sequences of data to provide
light-weight memory views which act like simple iterators but which
may hide more complex data structures.  For example, cells can be
stored in a memory-efficient graph structure which also provides fast
associations.  For simple iteration it is best to reuse that structure
while providing "familiar" iterators.

Some of the important data interfaces include:

* **wire** a segment of wire with physical endpoints and associations to channel and an index into its plane of wires.

* **cell** a region of space in the wire plane associated with one wire from each.

* **blob** a collection of cells which itself implements the cell data interface.

* **wire summary tiling** implement common queries on wires and cells.

* **wire plane and channel slices** implement access to information about charge in a slice in time across either a plane of wires or a detectors readout channels


Signal/Slot
===========

Wire Cell implements a pattern called "data flow programming" (DFP).
Instead of just calling functions with arguments and accepting their
returned results, DFP allows for high level "flow networks" to be
defined by connecting together functional *units* (objects) by
attaching one unit's output to another unit's input.

The reasons to follow the DFP paradigm are:

* minimize the amount of data that must be buffered into memory at any one time.

* reduce the coupling between units.

* support multiple computing architectures (single-processor,
  multi-processor, massive parallelism, CPU/GPU) with maximal code
  reuse.


Boost's Signal/Slot `boost::signals2` mechanism is used.  Components
which accept DFP input stream have a signal object which accepts
connections of slots matching a particular functional prototype.
Depending on the nature unit it may be that only a single connection
is allowed or that multiple ones can be made and possibly in a
predetermined order.

When the unit requires new data it "fires" (calls) its signal which
calls its slots and returns to the calling unit their return value(s).

Typically a slot is implemented as a `IData::pointer operator()()`
method which lacks any arguments.  But, in some cases such as
`WireCell::Fanout` slot arguments may be required and are passed in
through the signal call.  See also `WireCell::Addresser` which binds a
argument to pass at construction time which may be used to access a
particular "jack" of a `WireCell::Fanout`.

