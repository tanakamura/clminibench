package jp.main.Int.clminibench;

import com.example.clminibench.R;
import android.content.DialogInterface;
import android.support.v4.app.DialogFragment;
import android.app.Dialog;
import android.app.AlertDialog;
import android.os.Bundle;

public class SelectDeviceDialogFragment extends DialogFragment
{
    String devNames[];
    MainActivity ma;

    SelectDeviceDialogFragment(MainActivity ma,
                               String devNames[])
    {
        this.ma = ma;
        this.devNames = devNames;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle(R.string.seldevice_title);
        builder.setItems(devNames, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    ma.setDevice(which);
                }
            }
            );
        return builder.create();
    }
}