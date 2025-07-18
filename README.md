# bzzt
A for-fun, overly-ambitious project meant to create a spiritual successor to Tim Sweeney's ZZT from 1991.

## Background
bzzt is a project meant to satisfy my urge to build something really cool in C and satisfy my passion for weird, old software things.

ZZT and the community that still supports it today has fascinated me since I first discovered it some time in the late 90s. Though I've never
actually completed a ZZT world of my own, I've spent hundreds of hours playing the creations of other ZZTers. In recent years there's been
lots of really neat activity in the community ranging from a feature-rich [ZZT expansion](https://github.com/computirman/weave-zzt) to a
[complete](https://github.com/asiekierka/reconstruction-of-zzt/) disassembly and reverse engineering of the original Pascal source code with the blessing of Tim
Sweeney himself.

I wanted to do something cool too, so I've started writing bzzt (pronounced like it's spelled).

## What is it?

bzzt will aim to be a complete game development engine and player using ZZT as a base of inspiration and expanding on its capabilities for the modern age, while
keeping (mostly) in line with the vibe and limitations of ZZT.

## Goals and plans

One main goal is for bzzt to be completely "backwards-compatible" with ZZT worlds. I want you to be able to load up any ZZT world and play it just like the original engine,
but with the added options of various conveniences. Think more fluid player movement speed, support for higher resolutions, injecting new palettes and character sets to 
replace ZZT's CP437 font. Exporting original .ZZT worlds will be something I look into as well.

Aside from that, I want bzzt to be its own powerful game creation tool while maintaining some sensible limitations that appeal to the old-school spirit of ZZT that we all
know and love. I do not intend for bzzt to be another MegaZeux, but they may have some things in common. Therefore, bzzt will have 2 distinct "modes" - classic ZZT mode,
and the expanded bzzt mode.

Will I even finish this? Who knows.

## Current progress

Not much. Right now, I want to establish a baseline of rendering boards in a unique bzzt format. Soon, I plan to start parsing .ZZT worlds and converting ZZT boards into
bzzt boards that the engine can handle.

The feature that has the most work done as of now is bzzt's UI engine. I thought it would be a good idea to implement a full capable UI engine and use that to build up
the bzzt editor and player sidebar system. Currently working is a prototype system that's very similar to a modern game engine like Unity. You can read the complete
documentation for the UI engine and how building a custom UI will work in bzzt [here](https://github.com/VinceIP/bzzt/blob/main/docs/bzzt%20ui.txt).