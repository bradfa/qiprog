Hacking and Contributions
=========================

We use the same coding style and guidelines as the linux. Please see the linux
coding style guidelines for more details.


Moving data in and out
----------------------

As part of communicating with QiProg devices, it is often necessary to move data
to and from the host in streams.

##### Byte order is relevant

When putting data in a byte stream, or extracting data from a stream, make sure
you extract or insert the bytes in their correct order. Use the QiProg
byte-order conversion helpers for this. Check the Doxygen module with the same
name for more information.

##### Host byte order does NOT matter

NEVER EVER EVER EVER EVER EVER EVER EVER EVER do this:

	/* NEVER do this */
	var = *((uint32_t *)(ext_stream + 2))
	#if HOST_BYTE_ORDER == BIG_ENDIAN
	SWAP_BYTES(var);
	#endif

If you put the bytes in the correct order in the first place, this will never be
needed. For this reason you must only use the QiProg byte-order conversion
helpers, and use them as soon as data is in your control when reading, or just
before it leaves your control when writing. Never use GNU-specific extensions
such as le32toh. They violate all of the above rules.

The above example should look like:

	/* Instead, extract data in this manner */
	var = le32_to_h(ext_stream + 2);

The end result is less code, less clutter, more readability, and portability.

For more information see:
http://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html

##### Assume nothing about the structure of structures

Do not assume that the compiler will not pad structures, not even that it will
place the members in the orders in which you declare them. You must never use
compiler specific extensions such as "__attribute__ ((packed))", and never rely
on such extensions for achieving correct behavior. For this reason you must
always serialize/deserialize data going out or in. Never cast your raw data
pointer to a structure, or vice-versa.

	/* This is WRONG */
	struct my_packed_struct *le_struct = (void *)ext_stream;
	struct my_packed_struct my_host_struct;
	my_host_struct.member1 = le16_to_h(&(le_struct.member1));
	my_host_struct.member2 = le32_to_h(&(le_struct.member2));

Instead, manually place or extract each member to its proper place in the
stream.

	/* This is the CORRECT way to do it */
	my_host_struct.member1 = le16_to_h(ext_stream + 0);
	my_host_struct.member2 = le32_to_h(ext_stream + 2);

Although the wrong way is arguably more readable, it makes too many assumptions
about structures, and is therefore not portable, not even across different
versions of the same compiler.


Commit messages
---------------

For all aspects of Git to work the best, it's important to follow these simple
guidelines for commit messages:

1. The first line of the commit message has a short (less than 65 characters,
    absolute maximum is 75) summary
2. The second line is empty (no whitespace at all)
3. The third and any number of following lines contain a longer description of
    the commit as is neccessary, including relevant background information and
    quite possibly rationale for why the issue was solved in this particular
    way. These lines should never be longer than 75 characters.
4. The next line is empty (no whitespace at all)
5. A Signed-off-by: line according to the development guidelines

Please do not create Signed-off-by: manually because it is boring and
error-prone. Instead, please remember to always use git "commit -s" to have git
add your Signed-off-by: automatically.

* These guidelines have been adapted from http://www.coreboot.org/Git

Sign-off Procedure
------------------

We employ a similar sign-off procedure for QiProg as the Linux developers
do. Please add a note such as

> Signed-off-by: Random J Developer <random@developer.example.org>

to your email/patch if you agree with the following Developer's Certificate of
Origin 1.1. Patches without a Signed-off-by cannot be pushed to the official
repository. You have to use your real name in the Signed-off-by line and in any
copyright notices you add. Patches without an associated real name cannot be
committed!

<pre><code>
 Developer's Certificate of Origin 1.1:
 By making a contribution to this project, I certify that:

 (a) The contribution was created in whole or in part by me and I have
 the right to submit it under the open source license indicated in the file; or

 (b) The contribution is based upon previous work that, to the best of my
 knowledge, is covered under an appropriate open source license and I have the
 right under that license to submit that work with modifications, whether created
 in whole or in part by me, under the same open source license (unless I am
 permitted to submit under a different license), as indicated in the file; or

 (c) The contribution was provided directly to me by some other person who
 certified (a), (b) or (c) and I have not modified it; and

 (d) In the case of each of (a), (b), or (c), I understand and agree that
 this project and the contribution are public and that a record of the contribution
 (including all personal information I submit with it, including my sign-off) is
 maintained indefinitely and may be redistributed consistent with this project or the
 open source license indicated in the file.
</code></pre>

Note: The Developer's Certificate of Origin 1.1 is licensed under the terms of
the Creative Commons Attribution-ShareAlike 2.5 License.

* Guidelines adapted from http://www.coreboot.org/Development_Guidelines
