import React from 'react';
import type { LucideIcon } from 'lucide-react';

interface MetricCardProps {
  title: string;
  value: string | number;
  subValue?: string;
  icon: LucideIcon;
  progressPercent?: number;
  status?: 'normal' | 'warning' | 'danger' | 'success';
  trendLabel?: string;
}

export const MetricCard: React.FC<MetricCardProps> = ({
  title,
  value,
  subValue,
  icon: Icon,
  progressPercent,
  status = 'normal',
  trendLabel,
}) => {
  const getStatusColor = () => {
    switch (status) {
      case 'danger':
        return 'text-rose-600 border-rose-200 bg-rose-50/50';
      case 'warning':
        return 'text-amber-600 border-amber-200 bg-amber-50/50';
      case 'success':
        return 'text-emerald-600 border-emerald-200 bg-emerald-50/50';
      default:
        return 'text-slate-700 border-slate-200 bg-white';
    }
  };

  const getProgressColor = () => {
    switch (status) {
      case 'danger':
        return 'bg-rose-500';
      case 'warning':
        return 'bg-amber-500';
      case 'success':
        return 'bg-emerald-500';
      default:
        return 'bg-emerald-600';
    }
  };

  return (
    <div className={`p-4 rounded-lg border shadow-xs transition-all ${getStatusColor()}`}>
      <div className="flex items-center justify-between">
        <span className="text-xs font-semibold tracking-wider text-slate-500 uppercase">{title}</span>
        <div className="p-1.5 rounded-md bg-slate-100 text-slate-600">
          <Icon className="w-4 h-4" />
        </div>
      </div>

      <div className="mt-2 flex items-baseline justify-between">
        <span className="text-2xl font-bold tracking-tight font-mono text-slate-900">{value}</span>
        {trendLabel && (
          <span className="text-xs font-medium text-slate-500">{trendLabel}</span>
        )}
      </div>

      {subValue && (
        <p className="mt-1 text-xs text-slate-500 truncate">{subValue}</p>
      )}

      {typeof progressPercent === 'number' && (
        <div className="mt-3">
          <div className="w-full bg-slate-100 rounded-full h-1.5 overflow-hidden">
            <div
              className={`h-1.5 rounded-full transition-all duration-500 ${getProgressColor()}`}
              style={{ width: `${Math.min(100, Math.max(0, progressPercent))}%` }}
            />
          </div>
        </div>
      )}
    </div>
  );
};
