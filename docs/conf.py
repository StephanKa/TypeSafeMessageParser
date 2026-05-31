# Configuration file for the Sphinx documentation builder.

project = 'TypeSafeMessageParser'
copyright = '2024, TypeSafeMessageParser Contributors'
author = 'TypeSafeMessageParser Contributors'
release = '0.0.1'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.githubpages',
    'sphinx_copybutton',
    'sphinxcontrib.mermaid',
    'breathe',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Theme configuration
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'includehidden': True,
    'titles_only': False,
}

html_static_path = ['_static']

# Breathe configuration for C++ API docs
breathe_projects = {
    'TypeSafeMessageParser': '../build/xml',
}
breathe_default_project = 'TypeSafeMessageParser'

# Mermaid configuration
mermaid_version = '10.9.0'
