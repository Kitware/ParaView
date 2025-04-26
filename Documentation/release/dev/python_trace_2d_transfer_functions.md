## Remove unused 2D transfer functions from Python trace

Previously, 2D transfer functions would always appear in Python trace or Python state files that included rendering components, even when the 2D transfer functions were not used at all. The recent changes to proxy property defaults, however, allow us to also remove 2D transfer functions when they are unmodified/unused.

Now, 2D transfer functions are no longer present in the state file if they are unmodified. The following are examples of lines that are removed:

```python
# get 2D transfer function for 'RTData'
rTDataTF2D = GetTransferFunction2D('RTData')

```

And in the setter for the lookup table:

```python
    TransferFunction2D=rTDataTF2D,
```

Those lines of code were actually unnecessary anyways, because the `TransferFunction2D` for the lookup table was already set to that exact same object.
