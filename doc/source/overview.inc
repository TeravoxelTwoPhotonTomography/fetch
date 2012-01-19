.. sectionauthor::Nathan Clack <clackn@janelia.hhmi.org>

**Fetch** is a 3D-tiling microscope acquisition software package that currently
focuses on resonant scanning two-photon imaging.
                           
Right now, this document serves as a little notebook for the author; a roadmap
to the code that sometimes records what the hell he was thinking when he
implemented a design.  Hence, the focus here is on the code and not on hardware
or experimental details.  Details for *users* of the software should be written
on a wiki or something somewhere [#]_.

The software is written in C++, but some sections are exclusively C.  It was
written by first detailing the hardware implementation and tacking on a minimal
Win32 GUI.  At some point, the code became very cumbersome and so it was
rewritten.  The design is better now, but there are many warts.

At this point, *Fetch* is capable of being used in production, although there
are still some significant issues_.

.. [#]  In the author's opinion, it's best if the documentation is editable by
        the people it was intended for.  This way, any mistakes they encounter
        can be fixed without the need for the original writer to be involved.
        For users, it's great if they have an editable web-based document to
        edit (a wiki).

.. _issues: https://bitbucket.org/nclack/fetch/issues?status=new&status=open
