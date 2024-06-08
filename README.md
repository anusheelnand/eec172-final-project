# EEC 172 A03 Pong Final Project

This application recreates the traditional Pong game.
It uses the accelerometers of two CC3200s to move 
the paddles. Another CC3200's accelerometer is interfaced
over UART using the "controller" project.
A global high score is tracked using AWS IoT services.

## Usage and Publishing

This website uses GNU Make and Pandoc to facilitate the conversion of 
author-written markdown pages into a formatted and styled static HTML page.
It is meant for deployment on GitHub Pages, but can be deployed anywhere
that can host static pages.

**After the software dependencies are installed, *and you've written the page's
content in markdown*, you can build the webpage by simply invoking `make` 
in your terminal at the top level of the repository.**

Note that make will not do anything unless you've modified files in the 
`markdown` folder

### Usage

Create a repository from this template by following [these instructions](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-repository-from-a-template).

Then clone your repository locally and make edits to the template as needed.

### Publishing to GitHub Pages

You can make your repository publish itself automatically by following 
[these instructions](https://docs.github.com/en/pages/getting-started-with-github-pages/creating-a-github-pages-site).

## Dependecy Installation

Use of this template requires the following software:
- `pandoc`
- `make`
- `git` (for publishing to github)

You can check if you already have the commands by running the following
in your terminal:

  ```console
  $ pandoc --version
  $ make --version
  $ git --version
  ```

### Windows

Recommended installation method for windows if you don't already have it:

1.  Open PowerShell (v5.1 or later)
	- Yes, it needs to be PowerShell, not command line or cmd
	- It should _not_ be as an Administrator. Scoop is meant to be installed
	  at the user-level.

1.  Install [Scoop](https://scoop.sh/)
	```console
	$ Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
	$ Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression
	```

1.  Install anything you don't have:
	```console
	$ scoop install make
	$ scoop install pandoc
	$ scoop install git
	```

### MacOS

Recommended installation method for windows if you don't already have it:

1.  Follow the [lab setup instructions for mac](https://ucd-eec172.github.io/labs/lab-setup.html#macos-catalina-or-later)
    if you haven't already.

1.  Install anything you don't have:
	```console
	$ brew install make
	$ brew install pandoc
	$ brew install git
	```


## Acknowledgements

- Github CSS stylesheets provided by [sindresorhus](https://github.com/sindresorhus/github-markdown-css)
- Kushagra Tiwari and Shengmin Liu's EEC172 WQ24 final project *NutriSense*
