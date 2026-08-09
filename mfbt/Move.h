/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* C++11-style, but C++98-usable, "move references" implementation. */

#ifndef mozilla_Move_h
#define mozilla_Move_h

#include "mozilla/TypeTraits.h"

namespace mozilla {

/*
 * "Move" References
 *
 * [Once upon a time, C++11 rvalue references were not implemented by all the
 * compilers we cared about, so we invented mozilla::Move() (now called
 * OldMove()), which does something similar.  We're in the process of
 * transitioning away from this to pure stl (bug 896100).  Until that bug is
 * completed, this header will provide both mozilla::OldMove() and
 * mozilla::Move().]
 *
 *
 * Some types can be copied much more efficiently if we know the original's
 * value need not be preserved --- that is, if we are doing a "move", not a
 * "copy". For example, if we have:
 *
 *   Vector<T> u;
 *   Vector<T> v(u);
 *
 * the constructor for v must apply a copy constructor to each element of u ---
 * taking time linear in the length of u. However, if we know we will not need u
 * any more once v has been initialized, then we could initialize v very
 * efficiently simply by stealing u's dynamically allocated buffer and giving it
 * to v --- a constant-time operation, regardless of the size of u.
 *
 * Moves often appear in container implementations. For example, when we append
 * to a vector, we may need to resize its buffer. This entails moving each of
 * its extant elements from the old, smaller buffer to the new, larger buffer.
 * But once the elements have been migrated, we're just going to throw away the
 * old buffer; we don't care if they still have their values. So if the vector's
 * element type can implement "move" more efficiently than "copy", the vector
 * resizing should by all means use a "move" operation. Hash tables also need to
 * be resized.
 *
 * The details of the optimization, and whether it's worth applying, vary from
 * one type to the next. And while some constructor calls are moves, many really
 * are copies, and can't be optimized this way. So we need:
 *
 * 1) a way for a particular invocation of a copy constructor to say that it's
 *    really a move, and that the value of the original isn't important
 *    afterwards (although it must still be safe to destroy); and
 *
 * 2) a way for a type (like Vector) to announce that it can be moved more
 *    efficiently than it can be copied, and provide an implementation of that
 *    move operation.
 *
 * The OldMove(T&) function takes a reference to a T, and returns a MoveRef<T>
 * referring to the same value; that's (1). A MoveRef<T> is simply a reference
 * to a T, annotated to say that a copy constructor applied to it may move that
 * T, instead of copying it. Finally, a constructor that accepts an MoveRef<T>
 * should perform a more efficient move, instead of a copy, providing (2).
 *
 * The Move(T&) function takes a reference to a T and returns a T&&.  It acts
 * just like std::move(), which is not available on all our platforms.
 *
 * In new code, you should use Move(T&) and T&& instead of OldMove(T&) and
 * MoveRef<T>, where possible.
 *
 * Where we might define a copy constructor for a class C like this:
 *
 *   C(const C& rhs) { ... copy rhs to this ... }
 *
 * we would declare a move constructor like this:
 *
 *   C(C&& rhs) { .. move rhs to this ... }
 *
 * or, in the deprecated OldMove style:
 *
 *   C(MoveRef<C> rhs) { ... move rhs to this ... }
 *
 * And where we might perform a copy like this:
 *
 *   C c2(c1);
 *
 * we would perform a move like this:
 *
 *   C c2(Move(c1));
 *
 * or, in the deprecated OldMove style:
 *
 *   C c2(OldMove(c1));
 *
 * Note that MoveRef<T> implicitly converts to T&, so you can pass a MoveRef<T>
 * to an ordinary copy constructor for a type that doesn't support a special
 * move constructor, and you'll just get a copy.  This means that templates can
 * use Move whenever they know they won't use the original value any more, even
 * if they're not sure whether the type at hand has a specialized move
 * constructor.  If it doesn't, the MoveRef<T> will just convert to a T&, and
 * the ordinary copy constructor will apply.
 *
 * A class with a move constructor can also provide a move assignment operator,
 * which runs this's destructor, and then applies the move constructor to
 * *this's memory. A typical definition:
 *
 *   C& operator=(C&& rhs) {  // or |MoveRef<C> rhs|
 *     this->~C();
 *     new(this) C(rhs);
 *     return *this;
 *   }
 *
 * With that in place, one can write move assignments like this:
 *
 *   c2 = Move(c1); // or OldMove()
 *
 * This destroys c1, moves c1's value to c2, and leaves c1 in an undefined but
 * destructible state.
 *
 * This header file defines MoveRef, Move, and OldMove in the mozilla namespace.
 * It's up to individual containers to annotate moves as such, by calling Move
 * or OldMove; and it's up to individual types to define move constructors.
 * As we say, a move must leave the original in a "destructible" state. The
 * original's destructor will still be called, so if a move doesn't
 * actually steal all its resources, that's fine. We require only that the
 * move destination must take on the original's value; and that destructing
 * the original must not break the move destination.
 *
 * (Opinions differ on whether move assignment operators should deal with move
 * assignment of an object onto itself. It seems wise to either handle that
 * case, or assert that it does not occur.)
 *
 * Forwarding:
 *
 * Sometimes we want copy construction or assignment if we're passed an ordinary
 * value, but move construction if passed an rvalue reference. For example, if
 * our constructor takes two arguments and either could usefully be a move, it
 * seems silly to write out all four combinations:
 *
 *   C::C(X&  x, Y&  y) : x(x),       y(y)       { }
 *   C::C(X&  x, Y&& y) : x(x),       y(Move(y)) { }
 *   C::C(X&& x, Y&  y) : x(Move(x)), y(y)       { }
 *   C::C(X&& x, Y&& y) : x(Move(x)), y(Move(y)) { }
 *
 * To avoid this, C++11 has tweaks to make it possible to write what you mean.
 * The four constructor overloads above can be written as one constructor
 * template like so[0]:
 *
 *   template <typename XArg, typename YArg>
 *   C::C(XArg&& x, YArg&& y) : x(Forward<XArg>(x)), y(Forward<YArg>(y)) { }
 *
 * ("'Don't Repeat Yourself'? What's that?")
 *
 * This takes advantage of two new rules in C++11:
 *
 * One hint: if you're writing a move constructor where the type has members
 * that should be moved themselves, it's much nicer to write this:
 *
 *   C(MoveRef<C> c) : x(Move(c->x)), y(Move(c->y)) { }
 *
 * than the equivalent:
 *
 *   C(MoveRef<C> c) { new(&x) X(Move(c->x)); new(&y) Y(Move(c->y)); }
 *
 * especially since GNU C++ fails to notice that this does indeed initialize x
 * and y, which may matter if they're const.
 * - Second, Whereas C++ used to forbid references to references, C++11 defines
 *   'collapsing rules': 'T& &', 'T&& &', and 'T& &&' (that is, any combination
 *   involving an lvalue reference) now collapse to simply 'T&'; and 'T&& &&'
 *   collapses to 'T&&'.
 *
 *   Thus, in the call above, 'XArg&&' is 'X&& &&', collapsing to 'X&&'; and
 *   'YArg&&' is 'Y& &&', which collapses to 'Y &'. Because the arguments are
 *   declared as rvalue references to template arguments, the rvalue-ness
 *   "shines through" where present.
 *
 * Then, the 'Forward<T>' function --- you must invoke 'Forward' with its type
 * argument --- returns an lvalue reference or an rvalue reference to its
 * argument, depending on what T is. In our unified constructor definition, that
 * means that we'll invoke either the copy or move constructors for x and y,
 * depending on what we gave C's constructor. In our call, we'll move 'foo()'
 * into 'x', but copy 'yy' into 'y'.
 *
 * This header file defines Move and Forward in the mozilla namespace. It's up
 * to individual containers to annotate moves as such, by calling Move; and it's
 * up to individual types to define move constructors and assignment operators
 * when valuable.
 *
 * (C++11 says that the <utility> header file should define 'std::move' and
 * 'std::forward', which are just like our 'Move' and 'Forward'; but those
 * definitions aren't available in that header on all our platforms, so we
 * define them ourselves here.)
 *
 * 0. This pattern is known as "perfect forwarding".  Interestingly, it is not
 *    actually perfect, and it can't forward all possible argument expressions!
 *    There are two issues: one that's a C++11 issue, and one that's a legacy
 *    compiler issue.
 *
 *    The C++11 issue is that you can't form a reference to a bit-field.  As a
 *    workaround, assign the bit-field to a local variable and use that:
 *
 *      // C is as above
 *      struct S { int x : 1; } s;
 *      C(s.x, 0); // BAD: s.x is a reference to a bit-field, can't form those
 *      int tmp = s.x;
 *      C(tmp, 0); // OK: tmp not a bit-field
 *
 *    The legacy issue is that when we don't have true nullptr and must emulate
 *    it (gcc 4.4/4.5), forwarding |nullptr| results in an |int| or |long|
 *    forwarded reference.  But such a reference, even if its value is a null
 *    pointer constant expression, is not itself a null pointer constant
 *    expression.  This causes -Werror=conversion-null errors and pointer-to-
 *    integer comparison errors.  Until we always have true nullptr, users of
 *    forwarding methods must not pass |nullptr| to them.
 */
template<typename T>
class MoveRef
{
    T* pointer;

  public:
    explicit MoveRef(T& t) : pointer(&t) { }
    T& operator*() const { return *pointer; }
    T* operator->() const { return pointer; }
    operator T& () const { return *pointer; }
};

template<typename T>
inline MoveRef<T>
OldMove(T& t)
{
  return MoveRef<T>(t);
}

template<typename T>
inline MoveRef<T>
OldMove(const T& t)
{
  // With some versions of gcc, for a class C, there's an (incorrect) ambiguity
  // between the C(const C&) constructor and the default C(C&&) C++11 move
  // constructor, when the constructor is called with a const C& argument.
  //
  // This ambiguity manifests with the Move implementation above when Move is
  // passed const U& for some class U.  Calling Move(const U&) returns a
  // MoveRef<const U&>, which is then commonly passed to the U constructor,
  // triggering an implicit conversion to const U&.  gcc doesn't know whether to
  // call U(const U&) or U(U&&), so it wrongly reports a compile error.
  //
  // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50442 has since been fixed, so
  // this is no longer an issue for up-to-date compilers.  But there's no harm
  // in keeping it around for older compilers, so we might as well.  See also
  // bug 686280.
  return MoveRef<T>(const_cast<T&>(t));
}

/* Copy functions necessary for older compilers */
inline T&
Copy(T& t)
{
	return static_cast<T&>(t);
}

inline T&
ConstCopy(const T& t)
{
	return const_cast<T&>(t);

#if !defined (_MSC_VER) || _MSC_VER >= 1600
/**
 * Identical to std::Move(); this is necessary until our stlport supports
 * std::move().
 */
template<typename T>
inline typename RemoveReference<T>::Type&&
Move(T&& a)
{
  return static_cast<typename RemoveReference<T>::Type&&>(a);
}

/**
 * These two overloads are identidal to std::Forward(); they are necessary until
 * our stlport supports std::forward().
 */
template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type& a)
{
  return static_cast<T&&>(a);
}

template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type&& t)
{
  static_assert(!IsLvalueReference<T>::value,
                "misuse of Forward detected!  try the other overload");
  return static_cast<T&&>(t);
}
#endif

/** Swap |t| and |u| using move-construction if possible. */
template<typename T>
inline void
Swap(T& t, T& u)
{
  T tmp(OldMove(t));
  t = OldMove(u);
  u = OldMove(tmp);
}

} // namespace mozilla

#endif /* mozilla_Move_h */
