from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            "bioampexgpillclib._core",
            sources=["src/bioampexgpillclib/_core.c", "bioampexgpillClib.c"],
            include_dirs=["."],
            extra_compile_args=["-O3"],
        )
    ]
)
