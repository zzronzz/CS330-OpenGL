##CS330 OpenGL Final Project


This repository contains my final project for CS 330 Comp Graphics and Visualization. The project was created using C++ and OpenGL in Visual Studio 2022.

## Project Overview

For this project, I created a 3D desktop workstation scene. The scene includes a monitor, keyboard, desk surface, and desk lamp. The goal was to build a low-polygon 3D scene using basic shapes, textures, lighting, and camera controls.

## Scene Objects

The scene is made with several primitive shapes, including:

* Plane for the desk surface
* Boxes for the monitor, monitor frame, keyboard base, and keys
* Cylinders for the monitor stand, lamp base, and lamp stem
* Cone for the lampshade

## Features

This project includes:

* Low-polygon 3D objects
* Textures applied to the desk, monitor screen, keyboard, and other objects
* Multiple light sources
* A warm colored desk lamp light
* Perspective and orthographic viewing
* Camera movement using keyboard and mouse controls
* Organized transformation, texture, material, and lighting code

## Controls

The scene supports camera navigation using:

* `W`, `A`, `S`, `D` for forward, left, backward, and right movement
* `Q` and `E` for vertical movement
* Mouse movement for camera direction
* Mouse scroll for movement speed adjustment
* Keyboard toggle for switching between perspective and orthographic views

## Development Notes

During development, I adjusted the scene based on lighting results. A large background wall and large table plane caused a circular spotlight effect, so I removed the wall and made the desk surface smaller. I also added a desk lamp so the warm light source looked intentional within the scene.

## Tools Used

* Visual Studio 2022
* C++
* OpenGL
* GLSL shaders
* Texture image files

## Course

Southern New Hampshire University
CS 330 Comp Graphics and Visualization
