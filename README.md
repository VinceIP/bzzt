# bzzt
bzzt (pronounced like it sounds) is an early stage, in-development original game engine that also happens to run ZZT worlds.

## Background
bzzt is a project meant to satisfy my urge to build something really cool in C and satisfy my passion for weird, old software things.

ZZT and the community that still supports it today has fascinated me since I first discovered it some time in the late 90s. Though I've never
actually completed a ZZT world of my own, I've spent hundreds of hours playing the creations of other ZZTers. In recent years there's been
lots of really neat activity in the community ranging from a feature-rich [ZZT expansion](https://github.com/computirman/weave-zzt) to a
[complete](https://github.com/asiekierka/reconstruction-of-zzt/) disassembly and reverse engineering of the original Pascal source code with the blessing of Tim
Sweeney himself.

I wanted to do something cool too, so I've started writing bzzt (pronounced like it's spelled).

## What is it?

bzzt will aim to be a game development engine using ZZT as a baseline of inspiration/functionality, while
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

bzzt is currently able to load ZZT worlds, render boards, and has player movement with shooting and board transitions. Work is underway implementing ZZT's basic stat types.

In this early stage, bzzt already has a cool feature that expands upon what the original ZZT could do. Anyone can design a custom GUI using a simple YAML file (called a .bui) and create unique layouts with
custom colors, show variables during runtime, etc. It's somewhat inspired by modern game engines and UX design, supporting toggleable layers and anchoring properties.

## Building & Requirements

Note: file selection of zzt worlds not yet implemented :)

To build on Linux or Windows, ensure you have the latest releases of the following:
- gcc
- raylib
- libcyaml

Then run:
`make clean && make`